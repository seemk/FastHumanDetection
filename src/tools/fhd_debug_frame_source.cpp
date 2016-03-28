#include "fhd_debug_frame_source.h"
#include <stdlib.h>
#include <math.h>

fhd_debug_frame_source::fhd_debug_frame_source()
  : depth_data((uint16_t*)calloc(512 * 424, sizeof(uint16_t))) {
}

fhd_debug_frame_source::~fhd_debug_frame_source() {
  free(depth_data);
}

const uint16_t* fhd_debug_frame_source::get_frame() {
  for (int i = 0; i < 512 * 424; i++) {
    depth_data[i] = 500 + uint16_t(4200.f * fabsf(sinf(float(i / 1000.f) / (frame + 1))));
  }

  frame = (frame + 1) % 100;
  return depth_data;
}

int fhd_debug_frame_source::current_frame() const {
  return frame;
}

int fhd_debug_frame_source::total_frames() const {
  return 100;
}
