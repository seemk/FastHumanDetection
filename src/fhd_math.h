#pragma once

struct fhd_vec2 {
  float x;
  float y;
};

struct fhd_vec3 {
  float x;
  float y;
  float z;
};

fhd_vec2 fhd_vec2_sub(fhd_vec2 a, fhd_vec2 b);
fhd_vec2 fhd_vec2_normalize(fhd_vec2 v);
fhd_vec2 fhd_vec2_mul(fhd_vec2 a, float s);
fhd_vec2 fhd_vec2_mul_pcw(fhd_vec2 a, fhd_vec2 b);
float fhd_vec2_distance(fhd_vec2 a, fhd_vec2 b);
float fhd_vec2_length(fhd_vec2 v);

fhd_vec3 fhd_vec3_normalize(fhd_vec3 v);
float fhd_vec3_dot(fhd_vec3 a, fhd_vec3 b);
fhd_vec3 fhd_vec3_sub(fhd_vec3 a, fhd_vec3 b);
fhd_vec3 fhd_vec3_cross(fhd_vec3 u, fhd_vec3 v);

