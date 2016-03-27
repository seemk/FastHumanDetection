#include "fhd_segmentation.h"
#include <algorithm>

// Efficient Graph Based Image Segmentation
// by Pedro F. Felzenswalb, Daniel P.Huttenlocher
// Code taken from http://cs.brown.edu/~pff/segment/

inline float threshold_value(float size, float c) { return c / size; }

int fhd_segmentation_find(fhd_segmentation* u, int x) {
  int y = x;
  while (y != u->nodes[y].p) {
    y = u->nodes[y].p;
  }

  u->nodes[x].p = y;
  return y;
}

void fhd_segmentation_reset(fhd_segmentation* u) {
  u->num_nodes = u->num_initial_nodes;
  for (int i = 0; i < u->num_initial_nodes; i++) {
    u->nodes[i].size = 1;
    u->nodes[i].p = i;
  }
}

void fhd_segmentation_init(fhd_segmentation* u, int num_elements) {
  u->num_initial_nodes = num_elements;
  u->threshold = (float*)calloc(num_elements, sizeof(float));
  u->nodes = (fhd_seg_node*)calloc(num_elements, sizeof(fhd_seg_node));

  fhd_segmentation_reset(u);
}

void fhd_segmentation_destroy(fhd_segmentation* u) {
  free(u->nodes);
  free(u->threshold);
  free(u);
}

void fhd_segmentation_join(fhd_segmentation* u, int x, int y) {
  fhd_seg_node* nodes = u->nodes;
  if (nodes[x].rank > nodes[y].rank) {
    nodes[y].p = x;
    nodes[x].size += nodes[y].size;
  } else {
    nodes[x].p = y;
    nodes[y].size += nodes[x].size;

    if (nodes[x].rank == nodes[y].rank) {
      nodes[y].rank++;
    }
  }

  u->num_nodes--;
}

int fhd_segmentation_size(const fhd_segmentation* u, int x) {
  return u->nodes[x].size;
}

void fhd_segment_graph(fhd_segmentation* u, fhd_edge* edges, int num_edges,
                       float c, int min_size) {
  std::sort(edges, edges + num_edges, [](const fhd_edge& a, const fhd_edge& b) {
    return a.weight < b.weight;
  });

  for (int i = 0; i < u->num_initial_nodes; i++) {
    u->threshold[i] = threshold_value(1.f, c);
  }

  for (int i = 0; i < num_edges; i++) {
    fhd_edge* e = &edges[i];

    int a = fhd_segmentation_find(u, e->a);
    int b = fhd_segmentation_find(u, e->b);

    if (a != b) {
      if (e->weight <= u->threshold[a] && e->weight <= u->threshold[b]) {
        fhd_segmentation_join(u, a, b);
        a = fhd_segmentation_find(u, a);
        u->threshold[a] =
            e->weight + threshold_value(fhd_segmentation_size(u, a), c);
      }
    }
  }

  for (int i = 0; i < num_edges; i++) {
    fhd_edge* e = &edges[i];
    int a = fhd_segmentation_find(u, e->a);
    int b = fhd_segmentation_find(u, e->b);
    if (a != b && (fhd_segmentation_size(u, a) < min_size ||
                   fhd_segmentation_size(u, b) < min_size)) {
      fhd_segmentation_join(u, a, b);
    }
  }
}
