#pragma once

#include <stdint.h>

struct fhd_frame_source {
  virtual const uint16_t* get_frame() = 0;
  virtual void advance() {}
  virtual int current_frame() const = 0;
  virtual int total_frames() const = 0;
  virtual ~fhd_frame_source() {}
};
