#pragma once

#include "fhd_image.h"
#include "fhd_math.h"

struct fhd_hog_cell {
  float bins[9];
};

struct fhd_candidate {
  int candidate_width;
  int candidate_height;
  fhd_image depth;
  fhd_image_region depth_position;

  int num_cells;
  fhd_hog_cell* cells;

  int num_features;
  float* features;

  float weight;
};

struct fhd_result {
  int num_features;
  int human;
  float* features;
};

void fhd_hog_calculate_cells(const fhd_image* img, fhd_hog_cell* out);
void fhd_hog_create_features(const fhd_hog_cell* cells, float* features);
