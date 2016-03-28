#pragma once

#include "../fhd_frame_source.h"

struct fhd_debug_frame_source : fhd_frame_source {
  fhd_debug_frame_source();
  ~fhd_debug_frame_source();
  virtual const uint16_t* get_frame() override;

  int frame = 0;
  uint16_t* depth_data;
};
