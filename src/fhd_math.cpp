#include "fhd_math.h"
#include <math.h>

fhd_vec2 fhd_vec2_sub(fhd_vec2 a, fhd_vec2 b) {
  fhd_vec2 r;
  r.x = a.x - b.x;
  r.y = a.y - b.y;
  return r;
}

fhd_vec2 fhd_vec2_normalize(fhd_vec2 v) {
  const float a = sqrtf(v.x * v.x + v.y * v.y);
  return fhd_vec2{v.x / a, v.y / a};
}

fhd_vec2 fhd_vec2_mul(fhd_vec2 a, float s) { return {a.x * s, a.y * s}; }

fhd_vec2 fhd_vec2_mul_pcw(fhd_vec2 a, fhd_vec2 b) {
  return {a.x * b.x, a.y * b.y};
}

float fhd_vec2_distance(fhd_vec2 a, fhd_vec2 b) {
  return hypotf(b.x - a.x, b.y - a.y);
}

float fhd_vec2_length(fhd_vec2 v) { return sqrtf(v.x * v.x + v.y * v.y); }

fhd_vec3 fhd_vec3_normalize(fhd_vec3 v) {
  const float a = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  return fhd_vec3{v.x / a, v.y / a, v.z / a};
}

float fhd_vec3_dot(fhd_vec3 a, fhd_vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

fhd_vec3 fhd_vec3_sub(fhd_vec3 a, fhd_vec3 b) {
  fhd_vec3 r;
  r.x = a.x - b.x;
  r.y = a.y - b.y;
  r.z = a.z - b.z;
  return r;
}

fhd_vec3 fhd_vec3_cross(fhd_vec3 u, fhd_vec3 v) {
  const float t1 = u.x - u.y;
  const float t2 = v.y + v.z;
  const float t3 = u.x * v.z;
  const float t4 = t1 * t2 - t3;

  fhd_vec3 r;
  r.x = v.y * (t1 - u.z) - t4;
  r.y = u.z * v.x - t3;
  r.z = t4 - u.y * (v.x - t2);

  return r;
}
