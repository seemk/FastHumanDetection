#include "fhd_image.h"
#include "fhd_math.h"
#include <stdlib.h>
#include <assert.h>
#include <emmintrin.h>

void fhd_image_init(fhd_image* img, int w, int h) {
  img->width = w;
  img->height = h;
  img->len = w * h;
  img->pitch = w * sizeof(uint16_t);
  img->bytes = img->pitch * h;
  img->data = (uint16_t*)calloc(w * h, sizeof(uint16_t));
}

void fhd_image_destroy(fhd_image* img) {
  free(img->data);
  img->data = NULL;
}

void fhd_image_clear(fhd_image* img, uint16_t v) {
  for (int i = 0; i < img->len; i++) {
    img->data[i] = v;
  }
}

void fhd_image_map_values(fhd_image* img, uint16_t a, uint16_t b, uint16_t c,
                          uint16_t d) {
  for (int i = 0; i < img->len; i++) {
    uint16_t v = img->data[i];
    img->data[i] = fhd_map_range(v, a, b, c, d);
  }
}

void fhd_copy_sub_image_scale(const fhd_image* src,
                              const fhd_image_region* src_reg, fhd_image* dst,
                              const fhd_image_region* dst_reg) {
  const float x_ratio = float(src_reg->width) / float(dst_reg->width);
  const float y_ratio = float(src_reg->height) / float(dst_reg->height);

  for (int i = 0; i < dst_reg->height; i++) {
    int y = int(y_ratio * i);
    for (int j = 0; j < dst_reg->width; j++) {
      int x = int(x_ratio * j);
      int src_index = (src_reg->y + y) * src->width + (src_reg->x + x);

      int dst_offset = (i + dst_reg->y) * dst->width + (j + dst_reg->x);

      dst->data[dst_offset++] = src->data[src_index];
    }
  }
}

void fhd_copy_sub_image(const fhd_image* src, const fhd_image_region* src_reg,
                        fhd_image* dst, const fhd_image_region* dst_reg) {
  assert(dst_reg->width <= src_reg->width && dst_reg->height <= src_reg->height);
  for (int y = 0; y < dst_reg->height; y++) {
    for (int x = 0; x < dst_reg->width; x++) {
      int src_index = (src_reg->y + y) * src->width + (src_reg->x + x);
      int dst_offset = (dst_reg->y + y) * dst->width + (dst_reg->x + x);
      dst->data[dst_offset] = src->data[src_index];
    }
  }
}

void transpose_simd_8(const fhd_image* src, fhd_image* dst) {
  assert(src->width == dst->height && src->height == dst->width);
  assert(src->width % 8 == 0 && src->height % 8 == 0 && dst->width % 8 == 0 &&
         dst->height % 8 == 0);

  const int src_stride = src->width;
  const int dst_stride = dst->width;
  const uint16_t* src_data = src->data;
  uint16_t* dst_data = dst->data;
  for (int y = 0; y < src->height; y += 8) {
    for (int x = 0; x < src_stride; x += 8) {
      const int src_idx = y * src->width + x;
      const int dst_idx = x * dst->width + y;
      __m128i r0 = _mm_load_si128((const __m128i*)&src_data[src_idx]);
      __m128i r1 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride]);
      __m128i r2 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 2]);
      __m128i r3 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 3]);
      __m128i r4 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 4]);
      __m128i r5 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 5]);
      __m128i r6 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 6]);
      __m128i r7 =
          _mm_load_si128((const __m128i*)&src_data[src_idx + src_stride * 7]);

      __m128i v0 = _mm_unpacklo_epi16(r0, r1);
      __m128i v1 = _mm_unpacklo_epi16(r2, r3);
      __m128i v2 = _mm_unpacklo_epi16(r4, r5);
      __m128i v3 = _mm_unpacklo_epi16(r6, r7);
      __m128i v4 = _mm_unpackhi_epi16(r0, r1);
      __m128i v5 = _mm_unpackhi_epi16(r2, r3);
      __m128i v6 = _mm_unpackhi_epi16(r4, r5);
      __m128i v7 = _mm_unpackhi_epi16(r6, r7);

      __m128i c0 = _mm_unpacklo_epi32(v0, v1);
      __m128i c1 = _mm_unpackhi_epi32(v0, v1);
      __m128i c2 = _mm_unpacklo_epi32(v2, v3);
      __m128i c3 = _mm_unpackhi_epi32(v2, v3);
      __m128i c4 = _mm_unpacklo_epi32(v4, v5);
      __m128i c5 = _mm_unpackhi_epi32(v4, v5);
      __m128i c6 = _mm_unpacklo_epi32(v6, v7);
      __m128i c7 = _mm_unpackhi_epi32(v6, v7);

      __m128i d0 = _mm_unpacklo_epi64(c0, c2);
      __m128i d1 = _mm_unpackhi_epi64(c0, c2);
      __m128i d2 = _mm_unpacklo_epi64(c1, c3);
      __m128i d3 = _mm_unpackhi_epi64(c1, c3);
      __m128i d4 = _mm_unpacklo_epi64(c4, c6);
      __m128i d5 = _mm_unpackhi_epi64(c4, c6);
      __m128i d6 = _mm_unpacklo_epi64(c5, c7);
      __m128i d7 = _mm_unpackhi_epi64(c5, c7);

      _mm_store_si128((__m128i*)&dst_data[dst_idx], d0);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride], d1);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 2], d2);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 3], d3);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 4], d4);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 5], d5);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 6], d6);
      _mm_store_si128((__m128i*)&dst_data[dst_idx + dst_stride * 7], d7);
    }
  }
}

void fhd_image_flip_x(const fhd_image* src, fhd_image* dst) {
  assert(src->width == dst->width && src->height == dst->height);

  for (int y = 0; y < src->height; y++) {
    for (int x = 0; x < src->width; x++) {
      int src_idx = y * src->width + x;
      int dst_idx = y * src->width + (src->width - x - 1);
      dst->data[dst_idx] = src->data[src_idx];
    }
  }
}
