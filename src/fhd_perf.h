#pragma once

#include <stdint.h>

enum fhd_perf_record_type {
  pr_normalize_depth,
  pr_construct_pcl,
  pr_segment_depth,
  pr_construct_normals,
  pr_segment_normals,
  pr_construct_regions,
  pr_merge_regions,
  pr_copy_regions,
  pr_calc_hog,
  pr_create_features,
  pr_classify,
  PERF_RECORD_COUNT
};

static const char* const fhd_perf_record_names[PERF_RECORD_COUNT] = {
    "normalize depth",   "construct point cloud", "segment depth",
    "construct normals", "segment normals",       "construct regions",
    "merge regions",     "copy regions",          "calculate HOG",
    "create features",   "classify"};

struct fhd_perf_record {
  uint64_t cycles;
  uint64_t count;
  uint64_t avg_cycles;
};

uint64_t fhd_rdtsc();

struct fhd_timed_block {
  fhd_perf_record* record;
  uint64_t start = 0;

  fhd_timed_block(fhd_perf_record* r) : record(r), start(fhd_rdtsc()) {}

  ~fhd_timed_block() {
    uint64_t end = fhd_rdtsc();
    uint64_t cycles = end - start;
    record->cycles += cycles;
    record->count += 1;
    record->avg_cycles = record->cycles / record->count;
  }
};

#define FHD_TIMED_BLOCK(r) fhd_timed_block tblock_##__LINE__(r)
