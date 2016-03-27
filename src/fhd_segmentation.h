#pragma once

struct fhd_edge {
  int a;
  int b;
  float weight;
};

struct fhd_seg_node {
  int rank;
  int p;
  int size;
};

struct fhd_segmentation {
  int num_initial_nodes;
  int num_nodes;
  float* threshold;
  fhd_seg_node* nodes;
};

int fhd_segmentation_find(fhd_segmentation* u, int x);
void fhd_segmentation_reset(fhd_segmentation* u);
void fhd_segmentation_init(fhd_segmentation* u, int num_elements);
void fhd_segmentation_destroy(fhd_segmentation* u);
void fhd_segmentation_join(fhd_segmentation* u, int x, int y);
int fhd_segmentation_size(const fhd_segmentation* u, int x);
void fhd_segment_graph(fhd_segmentation* u, fhd_edge* edges, int num_edges,
                       float c, int min_size);
