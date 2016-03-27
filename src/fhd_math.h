#pragma once

const float F_PI = 3.14159265358979323846f;
const float F_PI_2 = F_PI * 0.5f;
const float F_PI_4 = F_PI * 0.25f;

struct fhd_vec2 {
  float x;
  float y;
};

struct fhd_vec3 {
  float x;
  float y;
  float z;
};

struct fhd_aabb {
  fhd_vec2 top_left;
  fhd_vec2 bot_right;
};

struct fhd_plane {
  fhd_vec3 n;
  float d;
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

void fhd_aabb_expand(fhd_aabb* bbox, fhd_vec2 point);
fhd_vec2 fhd_aabb_center(const fhd_aabb* bbox);
bool fhd_aabb_overlap(const fhd_aabb* a, const fhd_aabb* b);
fhd_vec2 fhd_aabb_size(const fhd_aabb* a);
fhd_aabb fhd_aabb_from_points(const fhd_vec2* points, int len);

float fhd_plane_point_dist(fhd_plane p, fhd_vec3 q);
fhd_plane fhd_make_plane(fhd_vec3 a, fhd_vec3 b, fhd_vec3 c);

template <typename T>
T fhd_map_range(T x, T a, T b, T c, T d) {
  return (x - a) * (d - c) / (b - a) + c;
}

template <typename T>
T fhd_clamp(T v, T min, T max) {
  if (v < min) return min;
  if (v > max) return max;

  return v;
}

float fhd_fast_atan2(float y, float x);

fhd_vec3 fhd_pcl_normal(const fhd_vec3* points, int len);
