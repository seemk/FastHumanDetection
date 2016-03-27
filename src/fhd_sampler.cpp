#include "fhd_sampler.h"
#include <stdlib.h>

// Based on http://gruenschloss.org/diag0m2/gendiag0m2.h

inline static int vdc(int bits) {
  bits = (bits << 16) | (bits >> 16);
  bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
  bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
  bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
  bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
  return bits;
}

int fhd_make_sampler(int m, int scale, fhd_index_2d* out, int len) {
  int n = 1 << m;
  int m2 = (m + 1) >> 1;
  int mask = ~-(1 << m2);
  int sqrtN = 1 << (m >> 1);
  int* d = (int*)calloc(sqrtN, sizeof(int));

  const double norm_scale = 1.0 / (1ULL << m);
  int to_write = n > len ? len : n;
  int dx;
  int dy;

  if (m & 1) {
    dx = dy = m >> 1;
    for (int k = 0; k < sqrtN; k++) {
      d[k] = (vdc(k) >> (32 - m)) + k;
    }
  } else {
    dx = (m >> 1) - 2;
    dy = m >> 1;
    for (int k = 0; k < sqrtN; k++) {
      d[k] = (vdc(k) >> (32 - m)) + (k >> 2) + (1 << dx);
    }
  }

  for (int i = 0; i < to_write; i++) {
    const int k = i >> m2;
    const int j = i & mask;

    int x = (d[k] + (j << dx)) & (n - 1);
    int y = k + (j << dy);
    
    const double tx = norm_scale * x * scale;
    const double ty = norm_scale * y * scale;
    out[i].x = int(tx);
    out[i].y = int(ty);
  }

  free(d);
  return to_write;
}
