#include "fhd_sqlite_source.h"
#include "sqlite3/sqlite3.h"
#include "fhd_math.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

fhd_sqlite_source::fhd_sqlite_source(const char* database) {
  int res = sqlite3_open_v2(database, &db, SQLITE_OPEN_READONLY, NULL);
  if (res != SQLITE_OK) {
    printf("failed to open %s: %s\n", database, sqlite3_errmsg(db));
  }

  res = sqlite3_prepare(
      db, "SELECT rowid, data FROM depth_frames WHERE rowid = (?)", -1,
      &frame_query, NULL);

  if (res != SQLITE_OK) {
    printf("failed to compile query: %s\n", sqlite3_errmsg(db));
  }

  if (!res) {
    sqlite3_stmt* count_stmt = NULL;
    sqlite3_prepare_v2(db, "SELECT count(*) FROM depth_frames", -1, &count_stmt,
                       NULL);

    res = sqlite3_step(count_stmt);

    if (res == SQLITE_ROW) {
      db_total_frames = sqlite3_column_int(count_stmt, 0);
      depth_data_len = 512 * 424;
      depth_data = (uint16_t*)calloc(depth_data_len, sizeof(uint16_t));
    }

    sqlite3_finalize(count_stmt);
  }
}

fhd_sqlite_source::~fhd_sqlite_source() {
  sqlite3_close_v2(db);
  free(depth_data);
}

const uint16_t* fhd_sqlite_source::get_frame() {
  if (!db || db_total_frames == 0) return NULL;

  advance();

  return depth_data;
}

void fhd_sqlite_source::advance() {
  sqlite3_reset(frame_query);
  sqlite3_bind_int(frame_query, 1, db_current_frame);
  int res;

  while ((res = sqlite3_step(frame_query)) == SQLITE_ROW) {
    const void* blob = sqlite3_column_blob(frame_query, 1);
    int bytes = sqlite3_column_bytes(frame_query, 1);
    const int req_depth_bytes = depth_data_len * sizeof(uint16_t);
    assert(bytes == req_depth_bytes);
    int bytes_to_copy = bytes;
    if (bytes_to_copy > req_depth_bytes) bytes_to_copy = req_depth_bytes;
    memcpy(depth_data, blob, bytes_to_copy);

    db_current_frame++;
    if (db_current_frame > db_total_frames) db_current_frame = 1;
  }

  if (res != SQLITE_DONE) {
    printf("failed to query frame: %s\n", sqlite3_errmsg(db));
  }
}

int fhd_sqlite_source::current_frame() const {
  return db_current_frame;
}

int fhd_sqlite_source::total_frames() const {
  return db_total_frames;
}
