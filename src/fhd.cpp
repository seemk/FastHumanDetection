#include "fhd.h"
#include "fhd_block_allocator.h"
#include "fhd_classifier.h"
#include "fhd_segmentation.h"
#include "fhd_kinect.h"
#include "pcg/pcg_basic.h"
#include <assert.h>
#include <time.h>
#include <algorithm>
#include <string.h>
#include <unordered_map>
#include <emmintrin.h>

#ifdef FHD_OMP
#include <omp.h>
#endif

static fhd_edge construct_depth_edge(int idx_a, float weight_a, int idx_b,
                                     float weight_b) {
  return fhd_edge{idx_a, idx_b, fabsf(weight_a - weight_b)};
}

void fhd_regions_remove(fhd_region* regions, int len, int idx) {
  for (int k = idx + 1; k < len; k++) {
    regions[k - 1] = regions[k];
  }
}

void fhd_construct_point_cloud(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_construct_pcl]);
  memset(fhd->point_cloud, 0, fhd->cells_len * sizeof(fhd_vec3));

  const int cell_sample_count = fhd->sampler_len;
  uint16_t* sample_buffer = fhd->cell_sample_buffer;

  const int cells_x = fhd->cells_x;
  const int cells_y = fhd->cells_y;
  const int source_w = fhd->source_w;
  const int cell_w = fhd->cell_w;
  const int cell_h = fhd->cell_h;
  const float cell_wf = fhd->cell_wf;
  const float cell_hf = fhd->cell_hf;

  for (int y = 0; y < cells_y; y++) {
    for (int x = 0; x < cells_x; x++) {
      for (int s = 0; s < cell_sample_count; s++) {
        fhd_index_2d p = fhd->sampler[s];
        int sx = x * cell_w + p.x;
        int sy = y * cell_h + p.y;
        sample_buffer[s] = fhd->normalized_source.data[sy * source_w + sx];
      }

      const int pivot = cell_sample_count / 2;
      std::nth_element(sample_buffer, sample_buffer + pivot,
                       sample_buffer + cell_sample_count);

      const uint16_t v = sample_buffer[pivot];

      const int idx = y * cells_x + x;
      fhd->downscaled_depth[idx] = v;

      if (v > 0) {
        fhd->point_cloud[idx] = fhd_depth_to_3d(
            float(v) / 1000.f, float(x) * cell_wf, float(y) * cell_hf);
      }
    }
  }
}

void fhd_perform_depth_segmentation(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_segment_depth]);
  // build graph
  for (int y = 0; y < fhd->cells_y; y++) {
    for (int x = 0; x < fhd->cells_x; x++) {
      if (x < fhd->cells_x - 1) {
        int idx_a = y * fhd->cells_x + x;
        int idx_b = y * fhd->cells_x + x + 1;
        const fhd_vec3* a = &fhd->point_cloud[idx_a];
        const fhd_vec3* b = &fhd->point_cloud[idx_b];

        fhd->depth_graph[fhd->num_depth_edges] =
            construct_depth_edge(idx_a, a->z, idx_b, b->z);
        fhd->num_depth_edges++;
      }

      if (y < fhd->cells_y - 1) {
        int idx_a = y * fhd->cells_x + x;
        int idx_b = (y + 1) * fhd->cells_x + x;
        const fhd_vec3* a = &fhd->point_cloud[idx_a];
        const fhd_vec3* b = &fhd->point_cloud[idx_b];

        fhd->depth_graph[fhd->num_depth_edges] =
            construct_depth_edge(idx_a, a->z, idx_b, b->z);
        fhd->num_depth_edges++;
      }
    }
  }

  fhd_segmentation_reset(fhd->depth_segmentation);
  fhd_segment_graph(fhd->depth_segmentation, fhd->depth_graph,
                    fhd->num_depth_edges, fhd->depth_segmentation_threshold,
                    fhd->min_depth_segment_size);
}

void fhd_construct_normals(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_construct_normals]);
  memset(fhd->normals, 0, fhd->cells_len * sizeof(fhd_vec3));
  fhd_vec3 pts_buffer[9];
  int pts_buffer_len = 0;
  for (int y = 1; y < fhd->cells_y - 1; y++) {
    for (int x = 1; x < fhd->cells_x - 1; x++) {
      const int idx = y * fhd->cells_x + x;
      const fhd_vec3 center = fhd->point_cloud[idx];
      if (center.z > 0.f) {
        int component = fhd_segmentation_find(fhd->depth_segmentation, idx);
        pts_buffer[pts_buffer_len++] = center;

#define ADD_POINT(neighbour_idx)                                       \
  {                                                                    \
    int neighbour_component =                                          \
        fhd_segmentation_find(fhd->depth_segmentation, neighbour_idx); \
    if (component == neighbour_component) {                            \
      const fhd_vec3 neighbour = fhd->point_cloud[neighbour_idx];      \
      if (neighbour.z > 0.f) {                                         \
        pts_buffer[pts_buffer_len++] = neighbour;                      \
      }                                                                \
    }                                                                  \
  }
        ADD_POINT((y - 1) * fhd->cells_x + (x - 1));
        ADD_POINT((y - 1) * fhd->cells_x + x);
        ADD_POINT((y - 1) * fhd->cells_x + (x + 1));
        ADD_POINT(y * fhd->cells_x + (x - 1));
        ADD_POINT(y * fhd->cells_x + (x + 1));
        ADD_POINT((y + 1) * fhd->cells_x + (x - 1));
        ADD_POINT((y + 1) * fhd->cells_x + x);
        ADD_POINT((y + 1) * fhd->cells_x + (x + 1));
#undef ADD_POINT
        if (pts_buffer_len > 1) {
          fhd->normals[idx] = fhd_pcl_normal(pts_buffer, pts_buffer_len);
        }
      }

      pts_buffer_len = 0;
    }
  }
}

void fhd_perform_normals_segmentation(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_segment_normals]);
  // construct normals graph
  for (int y = 0; y < fhd->cells_y; y++) {
    for (int x = 0; x < fhd->cells_x; x++) {
      if (x < fhd->cells_x - 1) {
        int idx_a = y * fhd->cells_x + x;
        int idx_b = y * fhd->cells_x + x + 1;
        fhd_vec3 a = fhd->normals[idx_a];
        fhd_vec3 b = fhd->normals[idx_b];

        fhd->normals_graph[fhd->num_normals_edges] =
            fhd_edge{idx_a, idx_b, acosf(fhd_vec3_dot(a, b))};
        fhd->num_normals_edges++;
      }

      if (y < fhd->cells_y - 1) {
        int idx_a = y * fhd->cells_x + x;
        int idx_b = (y + 1) * fhd->cells_x + x;
        fhd_vec3 a = fhd->normals[idx_a];
        fhd_vec3 b = fhd->normals[idx_b];

        fhd->normals_graph[fhd->num_normals_edges] =
            fhd_edge{idx_a, idx_b, acosf(fhd_vec3_dot(a, b))};
        fhd->num_normals_edges++;
      }
    }
  }

  fhd_segmentation_reset(fhd->normals_segmentation);
  fhd_segment_graph(fhd->normals_segmentation, fhd->normals_graph,
                    fhd->num_normals_edges, fhd->normal_segmentation_threshold,
                    fhd->min_normal_segment_size);
}

void fhd_construct_regions(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_construct_regions]);
  std::unordered_map<int64_t, fhd_region> regions;
  for (int y = 0; y < fhd->cells_y; y++) {
    for (int x = 0; x < fhd->cells_x; x++) {
      const int idx = y * fhd->cells_x + x;
      const fhd_vec3* p = &fhd->point_cloud[idx];
      if (p->z < 0.5f) {
        continue;
      }
      const int64_t depth_component =
          fhd_segmentation_find(fhd->depth_segmentation, idx);
      const int64_t normals_component =
          fhd_segmentation_find(fhd->normals_segmentation, idx);
      const int64_t key = (depth_component << 32) | normals_component;

      auto it = regions.find(key);

      fhd_region_point rp = {x, y, *p};
      if (it != regions.end()) {
        fhd_region* r = &it->second;
        if (p->x < r->sx) r->sx = p->x;
        if (p->x > r->ex) r->ex = p->x;
        if (p->y > r->sy) r->sy = p->y;
        if (p->y < r->ey) r->ey = p->y;

        fhd_aabb_expand(&r->bounds, fhd_vec2{float(x), float(y)});
        r->center.x = (r->sx + r->ex) * 0.5f;
        r->center.y = (r->sy + r->ey) * 0.5f;
        r->center.z = (p->z + r->n * r->center.z) / (r->n + 1.f);
        r->n += 1.f;
        r->points[r->points_len++] = rp;
      } else {
        fhd_region r;
        r.bounds.top_left.x = float(x);
        r.bounds.top_left.y = float(y);
        r.bounds.bot_right.x = float(x);
        r.bounds.bot_right.y = float(y);
        r.sx = p->x;
        r.sy = p->y;
        r.ex = p->x;
        r.ey = p->y;
        r.center.x = p->x;
        r.center.y = p->y;
        r.center.z = p->z;
        r.n = 1.f;
        r.points_len = 1;
        r.points = (fhd_region_point*)fhd_block_allocator_acquire(
            fhd->point_allocator);
        r.points[0] = rp;
        regions.insert(std::make_pair(key, r));
      }
    }
  }

  int largest_region = 0;
  for (auto& kv : regions) {
    fhd_region* r = &kv.second;
    const float width = r->ex - r->sx;
    const float height = r->sy - r->ey;

    if (r->points_len > largest_region) {
      largest_region = r->points_len;
    }

    if (r->n > fhd->min_region_size && width / height <= 2.f) {
      fhd->filtered_regions[fhd->filtered_regions_len++] = *r;
    } else {
      fhd_block_allocator_release(fhd->point_allocator, r->points);
    }

    if (fhd->filtered_regions_len == fhd->filtered_regions_capacity) {
      printf("Too many filtered regions\n");
      break;
    }
  }
}

float region_inlier_fraction(pcg32_random_t* rng, const fhd_region* r,
                             float max_plane_distance) {
  float inlier_fraction = 0.f;
  const uint32_t n_points = uint32_t(r->points_len);
  const float total = float(n_points);

  const int steps = 20;
  for (int k = 0; k < steps; k++) {
    // pick 3 random points
    uint32_t a_i, b_i, c_i;
    do {
      a_i = pcg32_boundedrand_r(rng, n_points);
      b_i = pcg32_boundedrand_r(rng, n_points);
      c_i = pcg32_boundedrand_r(rng, n_points);
    } while (a_i == b_i || a_i == c_i || b_i == c_i);
    const fhd_region_point* a = &r->points[a_i];
    const fhd_region_point* b = &r->points[b_i];
    const fhd_region_point* c = &r->points[c_i];

    const fhd_plane pi_k = fhd_make_plane(a->p_r, b->p_r, c->p_r);

    float num_fitting = 0.f;

    for (int i = 0; i < r->points_len; i++) {
      const fhd_region_point* cur_point = &r->points[i];
      const fhd_vec3 P = cur_point->p_r;
      const float distance = fabsf(fhd_plane_point_dist(pi_k, P));
      if (distance < max_plane_distance) {
        num_fitting += 1.f;
      }
    }

    float iter_fitting = num_fitting / total;
    if (iter_fitting > inlier_fraction) inlier_fraction = iter_fitting;
  }

  return inlier_fraction;
}

void fhd_merge_regions(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_merge_regions]);
  const float min_height = fhd->min_region_height;
  const float max_height = fhd->max_region_height;
  const float min_width = fhd->min_region_width;
  const float max_width = fhd->max_region_width;

  bool needs_merge = true;
  do {
    std::sort(
        fhd->filtered_regions,
        fhd->filtered_regions + fhd->filtered_regions_len,
        [](const fhd_region& a, const fhd_region& b) { return a.n > b.n; });

    int merges = 0;
    int len = fhd->filtered_regions_len;
    for (int i = 0; i < len; i++) {
      const fhd_region* r = &fhd->filtered_regions[i];
      const float width = r->ex - r->sx;
      const float height = r->sy - r->ey;
      const bool properly_sized = width >= min_width && width <= max_width &&
                                  height >= min_height && height <= max_height;

      if (!properly_sized) {
        const bool is_small = width < min_width || height < min_height;
#if FHD_PLANAR_REGIONS
        const float fraction =
            region_inlier_fraction(fhd->rng, r, fhd->ransac_max_plane_distance);
        if (is_small && fraction >= fhd->min_inlier_fraction) {
#else
        if (is_small) {
#endif
          // if (is_small && fraction >= fhd->min_inlier_fraction) {
          // find a region to merge into
          for (int j = 0; j < i; j++) {
            fhd_region* dst_r = &fhd->filtered_regions[j];
            const fhd_vec2 mu_xz_i = {r->center.x, r->center.z};
            const fhd_vec2 mu_xz_j = {dst_r->center.x, dst_r->center.z};

            const float sigma_xz =
                fhd_vec2_length(fhd_vec2_sub(mu_xz_i, mu_xz_j));
            const float dist_y = fabs(r->center.y - dst_r->center.y);

            if (sigma_xz < fhd->max_merge_distance &&
                dist_y < fhd->max_vertical_merge_distance) {
              const float total_points = r->n + dst_r->n;
              const float w_i = r->n / total_points;
              const float w_j = dst_r->n / total_points;
              assert(w_i + w_j == 1.f);
              fhd_aabb_expand(&dst_r->bounds, r->bounds.top_left);
              fhd_aabb_expand(&dst_r->bounds, r->bounds.bot_right);

              dst_r->sx = std::min(r->sx, dst_r->sx);
              dst_r->sy = std::max(r->sy, dst_r->sy);
              dst_r->ex = std::max(r->ex, dst_r->ex);
              dst_r->ey = std::min(r->ey, dst_r->ey);

              dst_r->center.x = w_i * r->center.x + w_j * dst_r->center.x;
              dst_r->center.y = w_i * r->center.y + w_j * dst_r->center.y;
              dst_r->center.z = w_i * r->center.z + w_j * dst_r->center.z;
              dst_r->n = total_points;

              for (int p = 0; p < r->points_len; p++) {
                dst_r->points[dst_r->points_len + p] = r->points[p];
              }
              dst_r->points_len += r->points_len;

              fhd_block_allocator_release(fhd->point_allocator, r->points);
              fhd_regions_remove(fhd->filtered_regions, len, i);
              i--;
              len--;
              merges++;

              break;
            }
          }
        } else {
          fhd_block_allocator_release(fhd->point_allocator, r->points);
          fhd_regions_remove(fhd->filtered_regions, len, i);
          i--;
          len--;
        }
      }
    }
    fhd->filtered_regions_len = len;
    needs_merge = merges > 0;
  } while (needs_merge);
}

void fhd_copy_regions(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_copy_regions]);
  memset(fhd->output_cell_indices, -1, fhd->cells_len * sizeof(int));
  fhd_image_clear(&fhd->output_depth, 0);

  const int cells_len = fhd->cells_len;
  for (int i = 0; i < fhd->filtered_regions_len; i++) {
    fhd_region* r = &fhd->filtered_regions[i];

    for (int j = 0; j < r->points_len; j++) {
      fhd_region_point p = r->points[j];
      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          int idx = (p.y + y) * fhd->cells_x + (p.x + x);
          if (idx > -1 && idx < cells_len) {
            fhd->output_cell_indices[idx] = 1;
          }
        }
      }
    }

    if (r->bounds.top_left.x >= 1.f) r->bounds.top_left.x -= 1.f;
    if (r->bounds.top_left.y >= 1.f) r->bounds.top_left.y -= 1.f;
    if (r->bounds.bot_right.x <= float(fhd->cells_x) - 1.f)
      r->bounds.bot_right.x += 1.f;
    if (r->bounds.bot_right.y <= float(fhd->cells_y) - 1.f)
      r->bounds.bot_right.y += 1.f;
  }

  for (int i = 0; i < fhd->cells_len; i++) {
    int v = fhd->output_cell_indices[i];
    if (v > -1) {
      int cell_x = i % fhd->cells_x;
      int cell_y = i / fhd->cells_x;

      int start_x = cell_x * fhd->cell_w;
      int start_y = cell_y * fhd->cell_h;

      for (int y = 0; y < fhd->cell_h; y++) {
        for (int x = 0; x < fhd->cell_w; x++) {
          const int depth_idx = (start_y + y) * fhd->source_w + (x + start_x);
          fhd->output_depth.data[depth_idx] =
              fhd->normalized_source.data[depth_idx];
        }
      }
    }
  }

  fhd->candidates_len = fhd->filtered_regions_len > fhd->candidates_capacity
                            ? fhd->candidates_capacity
                            : fhd->filtered_regions_len;
  for (int i = 0; i < fhd->candidates_len; i++) {
    const fhd_region* r = &fhd->filtered_regions[i];

    const float tl_x = r->bounds.top_left.x * fhd->cell_wf;
    const float tl_y = r->bounds.top_left.y * fhd->cell_hf;
    const float br_x = (r->bounds.bot_right.x * fhd->cell_wf) - 1.f;
    const float br_y = (r->bounds.bot_right.y * fhd->cell_hf) - 1.f;

    const float width = br_x - tl_x;
    const float height = br_y - tl_y;
    const fhd_vec2 center = fhd_kinect_coord_to_depth(r->center);

    const float z_tl_x = tl_x - center.x;
    const float z_tl_y = tl_y - center.y;
    const float z_br_x = br_x - center.x;
    const float z_br_y = br_y - center.y;

    const float sx_p = fabsf(float(FHD_HOG_WIDTH) * 0.5f / z_br_x);
    const float sx_n = fabsf(float(FHD_HOG_WIDTH) * 0.5f / z_tl_x);
    const float sy_p = fabsf(float(FHD_HOG_HEIGHT) * 0.5f / z_br_y);
    const float sy_n = fabsf(float(FHD_HOG_HEIGHT) * 0.5f / z_tl_y);

    fhd_image_region reg;
    reg.x = int(tl_x);
    reg.y = int(tl_y);
    reg.width = int(width);
    reg.height = int(height);

    fhd_candidate* candidate = &fhd->candidates[i];
    fhd_image_clear(&candidate->depth, 0);
    candidate->depth_position = reg;

    const float scale_factor =
        std::min(std::min(sy_p, sy_n), std::min(sx_p, sx_n));

    if (scale_factor < 1.0f) {
      const int dst_width = int(width * scale_factor);
      const int dst_height = int(height * scale_factor);
      fhd_image_region dst_reg;
      dst_reg.x = ((FHD_HOG_WIDTH - dst_width) / 2) + 1;
      dst_reg.y = ((FHD_HOG_HEIGHT - dst_height) / 2) + 1;
      dst_reg.width = dst_width;
      dst_reg.height = dst_height;
      fhd_copy_sub_image_scale(&fhd->output_depth, &reg, &candidate->depth,
                               &dst_reg);
    } else {
      fhd_image_region dst_reg;
      dst_reg.x = ((FHD_HOG_WIDTH - int(width)) / 2) + 1;
      dst_reg.y = ((FHD_HOG_HEIGHT - int(height)) / 2) + 1;
      dst_reg.width = int(width);
      dst_reg.height = int(height);
      fhd_copy_sub_image(&fhd->output_depth, &reg, &candidate->depth, &dst_reg);
    }
  }
}

void fhd_calculate_hog_cells(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_calc_hog]);

#ifdef FHD_OMP
#pragma omp parallel for num_threads(FHD_NUM_THREADS)
#endif
  for (int i = 0; i < fhd->candidates_len; i++) {
    fhd_candidate* candidate = &fhd->candidates[i];
    memset(candidate->cells, 0, candidate->num_cells * sizeof(fhd_hog_cell));
    fhd_hog_calculate_cells(&candidate->depth, candidate->cells);
  }
}

void fhd_create_features(fhd_context* fhd) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_create_features]);
  for (int i = 0; i < fhd->candidates_len; i++) {
    fhd_candidate* candidate = &fhd->candidates[i];
    fhd_hog_create_features(candidate->cells, candidate->features);
  }
}

void fhd_context_init(fhd_context* fhd, int source_w, int source_h, int cell_w,
                      int cell_h) {
  memset(fhd->perf_records, 0, sizeof(fhd->perf_records));
  fhd->source_w = source_w;
  fhd->source_h = source_h;
  fhd->source_len = source_w * source_h;
  fhd->source_wf = float(source_w);
  fhd->source_hf = float(source_h);
  fhd->cell_w = cell_w;
  fhd->cell_h = cell_h;
  fhd->cell_wf = float(cell_w);
  fhd->cell_hf = float(cell_h);

  fhd->cells_x = source_w / cell_w;
  fhd->cells_y = source_h / cell_h;
  fhd->cells_len = fhd->cells_x * fhd->cells_y;

  fhd->min_region_size = 9.f;
  fhd->max_merge_distance = 0.3f;
  fhd->max_vertical_merge_distance = 1.5f;
  fhd->min_inlier_fraction = 0.80f;
  fhd->min_region_height = 1.f;
  fhd->max_region_height = 3.f;
  fhd->min_region_width = 0.3f;
  fhd->max_region_width = 1.f;
  fhd->ransac_max_plane_distance = 0.05f;
  fhd->min_depth_segment_size = 4;
  fhd->min_normal_segment_size = 10;
  fhd->depth_segmentation_threshold = 2.f;
  fhd->normal_segmentation_threshold = 2.f;

  fhd->point_allocator =
      fhd_block_allocator_create(sizeof(fhd_region_point) * 2048, 512);

  fhd_image_init(&fhd->normalized_source, source_w, source_h);
  fhd->downscaled_depth = (uint16_t*)calloc(fhd->cells_len, sizeof(uint16_t));

  // The point cloud has sentinel on each side
  fhd->point_cloud = (fhd_vec3*)calloc(fhd->cells_len, sizeof(fhd_vec3));
  fhd->normals = (fhd_vec3*)calloc(fhd->cells_len, sizeof(fhd_vec3));

  fhd->depth_graph = (fhd_edge*)calloc(fhd->cells_len * 4, sizeof(fhd_edge));
  fhd->normals_graph = (fhd_edge*)calloc(fhd->cells_len * 4, sizeof(fhd_edge));

  fhd->normals_segmentation =
      (fhd_segmentation*)calloc(1, sizeof(fhd_segmentation));
  fhd_segmentation_init(fhd->normals_segmentation, fhd->cells_len);

  fhd->depth_segmentation =
      (fhd_segmentation*)calloc(1, sizeof(fhd_segmentation));
  fhd_segmentation_init(fhd->depth_segmentation, fhd->cells_len);

  fhd->num_depth_edges = 0;
  fhd->num_normals_edges = 0;

  fhd->filtered_regions_capacity = 64;
  fhd->filtered_regions_len = 0;
  fhd->filtered_regions =
      (fhd_region*)calloc(fhd->filtered_regions_capacity, sizeof(fhd_region));

  fhd->output_cell_indices = (int*)calloc(fhd->cells_len, sizeof(int));

  fhd_image_init(&fhd->output_depth, source_w, source_h);

  fhd->candidates_capacity = 8;
  fhd->candidates_len = 0;
  fhd->candidates =
      (fhd_candidate*)calloc(fhd->candidates_capacity, sizeof(fhd_candidate));

  for (int i = 0; i < fhd->candidates_capacity; i++) {
    fhd_candidate* candidate = &fhd->candidates[i];
    candidate->candidate_width = FHD_HOG_WIDTH;
    candidate->candidate_height = FHD_HOG_HEIGHT;
    fhd_image_init(&candidate->depth, FHD_HOG_WIDTH + 2, FHD_HOG_HEIGHT + 2);

    candidate->num_cells = (FHD_HOG_WIDTH / FHD_HOG_CELL_SIZE) *
                           (FHD_HOG_HEIGHT / FHD_HOG_CELL_SIZE);
    candidate->cells =
        (fhd_hog_cell*)calloc(candidate->num_cells, sizeof(fhd_hog_cell));

    candidate->num_features =
        FHD_HOG_BLOCKS_X * FHD_HOG_BLOCKS_Y * FHD_HOG_BLOCK_LEN;
    candidate->features =
        (float*)calloc(candidate->num_features, sizeof(float));
    candidate->weight = 0.f;
  }

  int sampler_n_pot = 4;
  fhd->sampler_len = (1 << sampler_n_pot);
  fhd->sampler = (fhd_index_2d*)calloc(fhd->sampler_len, sizeof(fhd_index_2d));
  fhd->cell_sample_buffer =
      (uint16_t*)calloc(fhd->sampler_len, sizeof(uint16_t));
  fhd_make_sampler(sampler_n_pot, fhd->cell_w, fhd->sampler, fhd->sampler_len);
  std::sort(fhd->sampler, fhd->sampler + fhd->sampler_len,
            [](fhd_index_2d a, fhd_index_2d b) {
              if (a.y == b.y) return a.x < b.x;
              return a.y < b.y;
            });

  fhd->rng = (pcg32_random_t*)calloc(1, sizeof(pcg32_random_t));

  uint64_t rng_seed = uint64_t(time(NULL));
  uint64_t rng_initseq = rng_seed >> 32;
  printf("seed: %lu seq: %lu\n", rng_seed, rng_initseq);
  pcg32_srandom_r(fhd->rng, rng_seed, rng_initseq);

  fhd->classifier = NULL;
}

void fhd_copy_depth(fhd_context* fhd, const uint16_t* source) {
  FHD_TIMED_BLOCK(&fhd->perf_records[pr_normalize_depth]);
  fhd->num_depth_edges = 0;
  fhd->num_normals_edges = 0;

  __m128i least = _mm_set1_epi16(499);
  __m128i most = _mm_set1_epi16(4501);
  for (int i = 0; i < fhd->source_len; i += 8) {
    __m128i vals = _mm_load_si128((const __m128i*)&source[i]);
    __m128i mask_least = _mm_cmpgt_epi16(vals, least);
    __m128i mask_most = _mm_cmplt_epi16(vals, most);
    __m128i mask_between = _mm_and_si128(mask_least, mask_most);
    __m128i between = _mm_and_si128(vals, mask_between);
    _mm_store_si128((__m128i*)&fhd->normalized_source.data[i], between);
  }
}

void fhd_run_pass(fhd_context* fhd, const uint16_t* source) {
  fhd->filtered_regions_len = 0;
  fhd_block_allocator_clear(fhd->point_allocator);
  fhd_copy_depth(fhd, source);
  fhd_construct_point_cloud(fhd);
  fhd_perform_depth_segmentation(fhd);
  fhd_construct_normals(fhd);
  fhd_perform_normals_segmentation(fhd);
  fhd_construct_regions(fhd);
  fhd_merge_regions(fhd);
  fhd_copy_regions(fhd);
  fhd_calculate_hog_cells(fhd);
  fhd_create_features(fhd);

  FHD_TIMED_BLOCK(&fhd->perf_records[pr_classify]);
  if (fhd->classifier) {
    for (int i = 0; i < fhd->candidates_len; i++) {
      fhd_candidate* candidate = &fhd->candidates[i];
      candidate->weight = fhd_classify(fhd->classifier, candidate);
    }
  }
}

void fhd_context_destroy(fhd_context* fhd) {
  fhd_block_allocator_destroy(fhd->point_allocator);
  fhd_image_destroy(&fhd->normalized_source);
  fhd_image_destroy(&fhd->output_depth);

  free(fhd->downscaled_depth);
  free(fhd->point_cloud);
  free(fhd->normals);
  free(fhd->depth_graph);
  free(fhd->normals_graph);
  free(fhd->normals_segmentation);
  free(fhd->depth_segmentation);
  free(fhd->filtered_regions);
  free(fhd->output_cell_indices);

  for (int i = 0; i < fhd->candidates_capacity; i++) {
    fhd_candidate* candidate = &fhd->candidates[i];
    fhd_image_destroy(&candidate->depth);
    free(candidate->cells);
    free(candidate->features);
  }

  free(fhd->candidates);
  free(fhd->sampler);
  free(fhd->cell_sample_buffer);
  free(fhd->rng);
}
