#pragma once

#include <stdint.h>

struct fhd_image {
  int width;
  int height;
  int len;
  int pitch;
  int bytes;
  uint16_t* data;
};

struct fhd_image_region {
  int x;
  int y;
  int width;
  int height;
};

void fhd_image_init(fhd_image* img, int w, int h);
void fhd_image_destroy(fhd_image* img);
void fhd_image_clear(fhd_image* img, uint16_t v);
// maps each fhd_image value from [a, b] to [c, d]
void fhd_image_map_values(fhd_image* img, uint16_t a, uint16_t b, uint16_t c,
                          uint16_t d);
void fhd_copy_sub_image(const fhd_image* src, const fhd_image_region* src_reg,
                        fhd_image* dst, const fhd_image_region* dst_reg);
void fhd_copy_sub_image_scale(const fhd_image* src,
                              const fhd_image_region* src_reg, fhd_image* dst,
                              const fhd_image_region* dst_reg);
void transpose_simd_8(const fhd_image* src, fhd_image* dst);
void fhd_image_flip_x(const fhd_image* src, fhd_image* dst);
