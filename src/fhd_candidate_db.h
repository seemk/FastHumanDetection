#pragma once

struct sqlite3;
struct sqlite3_stmt;
struct fhd_candidate;
struct fhd_result;

struct fhd_candidate_db {
  sqlite3* db;
  sqlite3_stmt* existing_query;
  sqlite3_stmt* insert_query;
  sqlite3_stmt* update_query;
  sqlite3_stmt* features_query;
};

void fhd_candidate_db_init(fhd_candidate_db* db, const char* db_name);
void fhd_candidate_db_close(fhd_candidate_db* db);
void fhd_candidate_db_add_candidate(fhd_candidate_db* db,
                                    const fhd_candidate* candidate,
                                    bool is_human);
int fhd_candidate_db_get_count(fhd_candidate_db* db);
int fhd_candidate_db_get_features(fhd_candidate_db* db, fhd_result* results,
                                  int max_results);
