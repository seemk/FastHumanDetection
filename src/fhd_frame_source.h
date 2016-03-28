#pragma once

#include <stdint.h>

struct fhd_frame_source {
  virtual const uint16_t* get_frame() = 0;
  virtual void advance() {}
  virtual ~fhd_frame_source() {}
};
