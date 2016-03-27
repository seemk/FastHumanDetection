#pragma once

#include "fhd_math.h"

const float FHD_KINECT_W = 512.f;
const float FHD_KINECT_H = 424.f;

fhd_vec2 fhd_kinect_coord_to_depth(fhd_vec3 p);
fhd_vec3 fhd_depth_to_3d(float depth, float x, float y);
