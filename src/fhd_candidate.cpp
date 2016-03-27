#include "fhd_candidate.h"
#include "fhd_config.h"
#include "fhd_math.h"
#include <math.h>
#include <float.h>
#include <assert.h>

void fhd_hog_calculate_cells(const fhd_image* img, fhd_hog_cell* out) {
  const int width = img->width;
  for (int y = 0; y < FHD_HOG_HEIGHT; y++) {
    for (int x = 0; x < FHD_HOG_WIDTH; x++) {
      const int idx = (y + 1) * width + (x + 1);
      const int16_t pv_y = int16_t(img->data[idx - width]);
      const int16_t pv_x = int16_t(img->data[idx - 1]);
      const int16_t nv_x = int16_t(img->data[idx + 1]);
      const int16_t nv_y = int16_t(img->data[idx + width]);

      fhd_vec2 grad;
      if (pv_x == 0 || nv_x == 0) {
        grad.x = 0.f;
      } else {
        grad.x = float(nv_x - pv_x) / 1000.f;
      }

      if (pv_y == 0 || nv_y == 0) {
        grad.y = 0.f;
      } else {
        grad.y = float(nv_y - pv_y) / 1000.f;
      }

      const float magnitude = fhd_vec2_length(grad);
      const float num_bins = float(FHD_HOG_BINS);
      const float angle =
          (fhd_fast_atan2(grad.y, grad.x) + F_PI) / (2.f * F_PI);
      const int bin = int(angle * num_bins + FLT_EPSILON) % FHD_HOG_BINS;
      assert(bin >= 0 && bin < FHD_HOG_BINS);

      const int cell_x = x / FHD_HOG_CELL_SIZE;
      const int cell_y = y / FHD_HOG_CELL_SIZE;
      const int cell_idx =
          cell_y * (FHD_HOG_WIDTH / FHD_HOG_CELL_SIZE) + cell_x;

      out[cell_idx].bins[bin] += magnitude;
    }
  }
}

void fhd_hog_create_features(const fhd_hog_cell* cells, float* features) {
  for (int y = 0; y < FHD_HOG_BLOCKS_Y; y++) {
    for (int x = 0; x < FHD_HOG_BLOCKS_X; x++) {
      int block_idx = y * FHD_HOG_BLOCKS_X + x;
      int features_offset = FHD_HOG_BLOCK_LEN * block_idx;

      float squared_sum = 1.f;
      int block_elem_idx = 0;
      for (int cy = 0; cy < FHD_HOG_BLOCK_SIZE; cy++) {
        for (int cx = 0; cx < FHD_HOG_BLOCK_SIZE; cx++) {
          int cell_x = x + cx;
          int cell_y = y + cy;
          int cell_idx = cell_y * (FHD_HOG_WIDTH / FHD_HOG_CELL_SIZE) + cell_x;
          const fhd_hog_cell* cell = &cells[cell_idx];
          for (int bin = 0; bin < FHD_HOG_BINS; bin++) {
            float v = cell->bins[bin];
            squared_sum += v * v;
            features[features_offset + block_elem_idx] = v;
            block_elem_idx++;
          }
        }
      }

      float normalization_factor = 1.f / sqrtf(squared_sum);
      for (int j = 0; j < FHD_HOG_BLOCK_LEN; j++) {
        const int idx = features_offset + j;
        const float v = (features[idx] * normalization_factor) * 2.f - 1.f;
        features[idx] = v;
      }
    }
  }
}
