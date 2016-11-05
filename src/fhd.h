#pragma once

#include "fhd_config.h"
#include "fhd_candidate.h"
#include "fhd_sampler.h"
#include "fhd_perf.h"
#include <stdint.h>

struct fhd_block_allocator;
struct fhd_segmentation;
struct fhd_classifier;
struct fhd_edge;
struct pcg_state_setseq_64;

struct fhd_region_point {
  int x;
  int y;
  fhd_vec3 p_r;
};

struct fhd_region {
  fhd_aabb bounds;  // x, y depth space coordinates

  // Bounding rectangle, metric space
  float sx;
  float sy;
  float ex;
  float ey;

  fhd_vec3 center;  // metric space
  float n;

  int points_len;
  fhd_region_point* points;
};

struct fhd_context {
  fhd_perf_record perf_records[PERF_RECORD_COUNT];

  int source_w;
  int source_h;
  int source_len;
  float source_wf;
  float source_hf;
  int cell_w;
  int cell_h;
  float cell_wf;
  float cell_hf;
  int cells_x;
  int cells_y;
  int cells_len;

  float min_region_size;
  float max_merge_distance;
  float max_vertical_merge_distance;
  float min_inlier_fraction;
  float min_region_height;
  float max_region_height;
  float min_region_width;
  float max_region_width;
  float ransac_max_plane_distance;
  int min_depth_segment_size;
  int min_normal_segment_size;
  float depth_segmentation_threshold;
  float normal_segmentation_threshold;

  fhd_block_allocator* point_allocator;

  fhd_image normalized_source;
  uint16_t* downscaled_depth;

  fhd_vec3* point_cloud;
  fhd_vec3* normals;

  fhd_edge* depth_graph;
  fhd_edge* normals_graph;

  fhd_segmentation* normals_segmentation;
  fhd_segmentation* depth_segmentation;

  int num_depth_edges;
  int num_normals_edges;

  int filtered_regions_capacity;
  int filtered_regions_len;
  fhd_region* filtered_regions;

  int* output_cell_indices;

  fhd_image output_depth;

  int candidates_capacity;
  int candidates_len;
  fhd_candidate* candidates;

  pcg_state_setseq_64* rng;

  int sampler_len;
  fhd_index_2d* sampler;
  uint16_t* cell_sample_buffer;
};

void fhd_context_init(fhd_context* fhd, int source_w, int source_h, int cell_w, int cell_h);
void fhd_run_pass(fhd_context* fhd, const uint16_t* source);
void fhd_run_classifier(fhd_context* fhd, const fhd_classifier* classifier);
void fhd_context_destroy(fhd_context* fhd);
