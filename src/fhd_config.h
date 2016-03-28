#pragma once

#define FHD_DEBUG_GRADIENTS 0
#define FHD_PLANAR_REGIONS 0

#ifndef FHD_NUM_THREADS
#define FHD_NUM_THREADS 4
#endif

const int FHD_HOG_WIDTH = 64;
const int FHD_HOG_HEIGHT = 128;
const int FHD_HOG_BLOCK_SIZE = 2;  // 2x2 cells per block
const int FHD_HOG_CELL_SIZE = 8;   // 8x8 pixels per cell
const int FHD_HOG_BINS = 9;
const int FHD_HOG_BLOCKS_X =
    (FHD_HOG_WIDTH - FHD_HOG_CELL_SIZE * 2) / FHD_HOG_CELL_SIZE + 1;
const int FHD_HOG_BLOCKS_Y =
    (FHD_HOG_HEIGHT - FHD_HOG_CELL_SIZE * 2) / FHD_HOG_CELL_SIZE + 1;
const int FHD_HOG_BLOCK_LEN =
    FHD_HOG_BINS * FHD_HOG_BLOCK_SIZE * FHD_HOG_BLOCK_SIZE;
