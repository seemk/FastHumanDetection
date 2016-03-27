#include "fhd_kinect.h"

const float FHD_KINECT_CX = 255.8f;
const float FHD_KINECT_CY = 203.7f;
const float FHD_KINECT_FX = 364.7f;
const float FHD_KINECT_FY = 366.1f;

fhd_vec2 fhd_kinect_coord_to_depth(fhd_vec3 p) {
  fhd_vec2 r;
  r.x =
      fhd_clamp((p.x * FHD_KINECT_FX) / p.z + FHD_KINECT_CX, 0.f, FHD_KINECT_W);
  r.y = fhd_clamp(-(p.y * FHD_KINECT_FY) / p.z + FHD_KINECT_CY, 0.f,
                  FHD_KINECT_H);
  return r;
};

fhd_vec3 fhd_depth_to_3d(float depth, float x, float y) {
  return {((x - FHD_KINECT_CX) / FHD_KINECT_FX) * depth,
          ((y - FHD_KINECT_CY) / FHD_KINECT_FY) * -depth, depth};
}
