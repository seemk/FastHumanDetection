#include "fhd_candidate_db.h"
#include "fhd_candidate.h"
#include "fhd_hash.h"
#include "sqlite3/sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool fhd_db_check_existing(fhd_candidate_db* db, int64_t hash) {
  sqlite3_reset(db->existing_query);
  sqlite3_bind_int64(db->existing_query, 1, hash);

  int res = sqlite3_step(db->existing_query);

  if (res == SQLITE_ROW) {
    int count = sqlite3_column_int(db->existing_query, 0);
    return count > 0;
  }

  return false;
}

bool fhd_db_insert_candidate(fhd_candidate_db* db, int64_t hash,
                             const fhd_image* depth, float* features,
                             int num_features, bool is_human) {
  sqlite3_reset(db->insert_query);
  sqlite3_bind_int64(db->insert_query, 1, hash);
  sqlite3_bind_int(db->insert_query, 2, depth->width);
  sqlite3_bind_int(db->insert_query, 3, depth->height);
  sqlite3_bind_blob(db->insert_query, 4, depth->data, depth->bytes,
                    SQLITE_STATIC);
  sqlite3_bind_blob(db->insert_query, 5, features, num_features * sizeof(float),
                    SQLITE_STATIC);
  sqlite3_bind_int(db->insert_query, 6, int(is_human));

  int res = sqlite3_step(db->insert_query);

  return res == SQLITE_DONE;
}

bool fhd_db_update_candidate(fhd_candidate_db* db, int64_t hash,
                             const fhd_image* depth, float* features,
                             int num_features, bool is_human) {
  sqlite3_reset(db->update_query);
  sqlite3_bind_int(db->update_query, 1, depth->width);
  sqlite3_bind_int(db->update_query, 2, depth->height);
  sqlite3_bind_blob(db->update_query, 3, depth->data, depth->bytes,
                    SQLITE_STATIC);
  sqlite3_bind_blob(db->update_query, 4, features, num_features * sizeof(float),
                    SQLITE_STATIC);
  sqlite3_bind_int(db->update_query, 5, int(is_human));
  sqlite3_bind_int64(db->update_query, 6, hash);

  int res = sqlite3_step(db->update_query);

  return res == SQLITE_DONE;
}

void fhd_db_upsert_candidate(fhd_candidate_db* db, const fhd_image* depth,
                             float* features, int num_features, bool is_human) {
  int64_t hash = int64_t(fhd_fnv1a_hash(depth->data, depth->bytes));

  bool already_exists = fhd_db_check_existing(db, hash);

  if (already_exists) {
    fhd_db_update_candidate(db, hash, depth, features, num_features, is_human);
  } else {
    fhd_db_insert_candidate(db, hash, depth, features, num_features, is_human);
  }
}

// candidates;
// hash:integer,
// width:integer,
// height:integer,
// depth:blob,
// features:blob,
// human:integer

void fhd_candidate_db_init(fhd_candidate_db* db, const char* db_name) {
  int res = sqlite3_open(db_name, &db->db);
  if (res != SQLITE_OK) {
    printf("failed to open %s: %s\n", db_name, sqlite3_errmsg(db->db));
    return;
  }

  char* err_msg = NULL;
  res = sqlite3_exec(
      db->db,
      "CREATE TABLE IF NOT EXISTS candidates (hash integer primary key not "
      "null, width integer not null, height integer not null, "
      "depth blob not null, features blob not null, human integer)",
      NULL, NULL, &err_msg);

  if (err_msg) {
    printf("%s\n", err_msg);
    sqlite3_free(err_msg);
  }

  res = sqlite3_prepare_v2(db->db,
                           "SELECT count(*) FROM candidates WHERE hash = (?)",
                           -1, &db->existing_query, NULL);

  auto sqlite_check = [=](int code) {
    if (code != SQLITE_OK) {
      printf("failed to compile query: %s\n", sqlite3_errmsg(db->db));
    }
  };

  sqlite_check(res);

  res = sqlite3_prepare_v2(db->db,
                           "INSERT INTO candidates VALUES (?, ?, ?, ?, ?, ?)",
                           -1, &db->insert_query, NULL);

  sqlite_check(res);

  res = sqlite3_prepare_v2(db->db,
                           "UPDATE candidates SET width = ?, height = ?, depth "
                           "= ?, features = ?, human = ? WHERE hash = ?",
                           -1, &db->update_query, NULL);

  sqlite_check(res);

  res = sqlite3_prepare_v2(db->db, "SELECT features, human FROM candidates", -1,
                           &db->features_query, NULL);

  sqlite_check(res);
}

void fhd_candidate_db_close(fhd_candidate_db* db) {
  if (!db) return;

  sqlite3_finalize(db->existing_query);
  sqlite3_finalize(db->insert_query);
  sqlite3_close_v2(db->db);
}

void fhd_candidate_db_add_candidate(fhd_candidate_db* db,
                                    const fhd_candidate* candidate,
                                    bool is_human) {
  int width = candidate->candidate_width;
  int height = candidate->candidate_height;
  fhd_image candidate_img;
  fhd_image flipped_depth;
  fhd_image_init(&candidate_img, width, height);
  fhd_image_init(&flipped_depth, width, height);

  fhd_image_region src_reg;
  src_reg.x = 1;
  src_reg.y = 1;
  src_reg.width = candidate->depth.width - 2;
  src_reg.height = candidate->depth.height - 2;

  fhd_image_region dst_reg;
  dst_reg.x = 0;
  dst_reg.y = 0;
  dst_reg.width = width;
  dst_reg.height = height;

  fhd_copy_sub_image(&candidate->depth, &src_reg, &candidate_img, &dst_reg);

  fhd_db_upsert_candidate(db, &candidate_img, candidate->features,
                          candidate->num_features, is_human);

  fhd_image flipped_orig_depth;
  fhd_image_init(&flipped_orig_depth, candidate->depth.width,
                 candidate->depth.height);
  fhd_image_flip_x(&candidate->depth, &flipped_orig_depth);
  fhd_copy_sub_image(&flipped_orig_depth, &src_reg, &flipped_depth, &dst_reg);

  fhd_hog_cell* cells =
      (fhd_hog_cell*)calloc(candidate->num_cells, sizeof(fhd_hog_cell));
  float* features = (float*)calloc(candidate->num_features, sizeof(float));

  fhd_hog_calculate_cells(&flipped_orig_depth, cells);
  fhd_hog_create_features(cells, features);

  fhd_db_upsert_candidate(db, &flipped_depth, features, candidate->num_features,
                          is_human);

  free(features);
  free(cells);

  fhd_image_destroy(&flipped_orig_depth);
  fhd_image_destroy(&flipped_depth);
  fhd_image_destroy(&candidate_img);
}

int fhd_candidate_db_get_count(fhd_candidate_db* db) {
  const auto cb = [](void* usr, int col_count, char** data, char**) {
    if (col_count > 0) {
      int count = strtol(data[0], NULL, 10);
      *(int*)usr = count;
    }
    return 0;
  };

  int count = 0;
  char* err_msg = NULL;
  sqlite3_exec(db->db, "SELECT count(*) from candidates", cb, &count, &err_msg);

  if (err_msg) {
    printf("%s\n", err_msg);
    sqlite3_free(err_msg);
  }

  return count;
}

int fhd_candidate_db_get_features(fhd_candidate_db* db, fhd_result* results,
                                  int max_results) {
  sqlite3_reset(db->features_query);

  int idx = 0;
  while (sqlite3_step(db->features_query) == SQLITE_ROW && idx < max_results) {
    const void* features = sqlite3_column_blob(db->features_query, 0);
    int num_bytes = sqlite3_column_bytes(db->features_query, 0);
    int is_human = sqlite3_column_int(db->features_query, 1);
    memcpy(results[idx].features, features, num_bytes);
    results[idx].num_features = num_bytes / sizeof(float);
    results[idx].human = is_human;
    idx++;
  }

  return idx;
}
