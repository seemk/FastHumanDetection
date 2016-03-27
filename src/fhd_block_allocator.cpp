#include "fhd_block_allocator.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

struct fhd_alloc_slot {
  void* ptr;
  int idx;
};

struct fhd_block_allocator {
  int block_size;
  int num_bytes;
  int num_blocks;
  uint8_t* memory;
  int free_indices_len;
  int* free_indices;

  int full_slots_len;
  fhd_alloc_slot* full_slots;
};

fhd_block_allocator* fhd_block_allocator_create(int block_size,
                                                int num_blocks) {
  fhd_block_allocator* block_alloc =
      (fhd_block_allocator*)calloc(1, sizeof(fhd_block_allocator));

  block_alloc->block_size = block_size;
  block_alloc->num_bytes = block_size * num_blocks;
  block_alloc->num_blocks = num_blocks;
  block_alloc->memory = (uint8_t*)calloc(block_alloc->num_bytes, 1);
  block_alloc->free_indices_len = num_blocks;
  block_alloc->free_indices = (int*)calloc(num_blocks, sizeof(int));
  block_alloc->full_slots_len = 0;
  block_alloc->full_slots =
      (fhd_alloc_slot*)calloc(num_blocks, sizeof(fhd_alloc_slot));

  for (int i = 0; i < num_blocks; i++) {
    block_alloc->free_indices[i] = i;
  }

  return block_alloc;
}

void fhd_block_allocator_destroy(fhd_block_allocator* block_alloc) {
  free(block_alloc->memory);
  free(block_alloc->free_indices);
  free(block_alloc->full_slots);
  free(block_alloc);
}

void* fhd_block_allocator_acquire(fhd_block_allocator* block_alloc) {
  if (block_alloc->free_indices_len == 0) return NULL;

  const int free_idx =
      block_alloc->free_indices[block_alloc->free_indices_len - 1];
  const int offset = free_idx * block_alloc->block_size;

  uint8_t* block = block_alloc->memory + offset;

  const fhd_alloc_slot slot = {block, free_idx};
  block_alloc->full_slots[block_alloc->full_slots_len++] = slot;

  block_alloc->free_indices_len--;

  return block;
}

void fhd_block_allocator_release(fhd_block_allocator* block_alloc, void* ptr) {
  for (int i = 0; i < block_alloc->full_slots_len; i++) {
    fhd_alloc_slot slot = block_alloc->full_slots[i];
    if (ptr == slot.ptr) {
      block_alloc->free_indices[block_alloc->free_indices_len] = slot.idx;

      for (int j = i; j < block_alloc->full_slots_len - 1; j++) {
        block_alloc->full_slots[j] = block_alloc->full_slots[j + 1];
      }

      block_alloc->free_indices_len++;
      block_alloc->full_slots_len--;

      return;
    }
  }
}

void fhd_block_allocator_clear(fhd_block_allocator* block_alloc) {
  for (int i = 0; i < block_alloc->full_slots_len; i++) {
    block_alloc->free_indices[block_alloc->free_indices_len++] =
        block_alloc->full_slots[i].idx;
  }

  block_alloc->full_slots_len = 0;
}
