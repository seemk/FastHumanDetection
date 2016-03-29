#pragma once

#include "fhd_frame_source.h"
#include <stdlib.h>

struct sqlite3;
struct sqlite3_stmt;

struct fhd_sqlite_source : fhd_frame_source {
  fhd_sqlite_source(const char* database);
  ~fhd_sqlite_source();
  void advance() override;
  int current_frame() const override;
  int total_frames() const override;
  const uint16_t* get_frame() override;

  sqlite3* db = NULL;
  sqlite3_stmt* frame_query = NULL;

  int db_total_frames = 0;
  int db_current_frame = 1;
  int depth_data_len = 0;
  uint16_t* depth_data = NULL;
};
