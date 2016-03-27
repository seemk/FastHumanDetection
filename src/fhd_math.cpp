#include "fhd_math.h"
#include <math.h>
#include <float.h>
#include <assert.h>
#include <Eigen/Dense>

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

void fhd_aabb_expand(fhd_aabb* bbox, fhd_vec2 point) {
  if (point.x < bbox->top_left.x)
    bbox->top_left.x = point.x;
  else if (point.x > bbox->bot_right.x)
    bbox->bot_right.x = point.x;

  if (point.y < bbox->top_left.y)
    bbox->top_left.y = point.y;
  else if (point.y > bbox->bot_right.y)
    bbox->bot_right.y = point.y;
}

fhd_vec2 fhd_aabb_center(const fhd_aabb* bbox) {
  return {(bbox->bot_right.x + bbox->top_left.x) * 0.5f,
          (bbox->bot_right.y + bbox->top_left.y) * 0.5f};
}

bool fhd_aabb_overlap(const fhd_aabb* a, const fhd_aabb* b) {
  if (a->bot_right.x < b->top_left.x || a->top_left.x > b->bot_right.x)
    return false;
  if (a->bot_right.y < b->top_left.y || a->top_left.y > b->bot_right.y)
    return false;

  return true;
}

fhd_vec2 fhd_aabb_size(const fhd_aabb* a) {
  fhd_vec2 r;
  r.x = a->bot_right.x - a->top_left.x;
  r.y = a->bot_right.y - a->top_left.y;
  return r;
}

fhd_aabb fhd_aabb_from_points(const fhd_vec2* points, int len) {
  assert(len >= 2);

  fhd_aabb bbox = {{FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX}};

  for (int i = 0; i < len; i++) {
    fhd_aabb_expand(&bbox, points[i]);
  }

  return bbox;
}

float fhd_plane_point_dist(fhd_plane p, fhd_vec3 q) {
  return (fhd_vec3_dot(p.n, q) - p.d) / fhd_vec3_dot(p.n, p.n);
}

fhd_plane fhd_make_plane(fhd_vec3 a, fhd_vec3 b, fhd_vec3 c) {
  fhd_plane p;
  p.n = fhd_vec3_normalize(
      fhd_vec3_cross(fhd_vec3_sub(b, a), fhd_vec3_sub(c, a)));
  p.d = fhd_vec3_dot(p.n, a);
  return p;
}

float fhd_fast_atan2(float y, float x) {
  const float THRQTR_PI = 3.f * F_PI_4;
  float r, angle;
  float abs_y = fabs(y) + 1e-10f;
  if (x < 0.0f) {
    r = (x + abs_y) / (abs_y - x);
    angle = THRQTR_PI;
  } else {
    r = (x - abs_y) / (x + abs_y);
    angle = F_PI_4;
  }
  angle += (0.1963f * r * r - 0.9817f) * r;
  if (y < 0.0f)
    return -angle;
  else
    return angle;
}

fhd_vec3 fhd_pcl_normal(const fhd_vec3* points, int len) {
  Eigen::Matrix3f A = Eigen::Matrix3f::Zero();
  Eigen::Vector3f b = Eigen::Vector3f::Zero();
  for (int i = 0; i < len; i++) {
    const fhd_vec3 v = points[i];
    A(0, 0) += v.x * v.x;
    A(0, 1) += v.x * v.y;
    A(0, 2) += v.x;
    A(1, 0) += v.x * v.y;
    A(1, 1) += v.y * v.y;
    A(1, 2) += v.y;
    A(2, 0) += v.x;
    A(2, 1) += v.y;

    b(0) += v.x * v.z;
    b(1) += v.y * v.z;
    b(2) += v.z;
  }

  A(2, 2) += float(len);
  Eigen::Vector3f solution = A.llt().solve(b);
  return fhd_vec3_normalize(fhd_vec3{solution(0), solution(1), solution(2)});
}
