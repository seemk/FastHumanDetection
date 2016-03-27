#pragma once

struct fhd_index_2d {
  int x;
  int y;
};

int fhd_make_sampler(int m, int scale, fhd_index_2d* out, int len);
