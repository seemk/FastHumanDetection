#pragma once

struct fhd_block_allocator;

fhd_block_allocator* fhd_block_allocator_create(int block_size, int num_blocks);
void fhd_block_allocator_destroy(fhd_block_allocator* block_alloc);
void* fhd_block_allocator_acquire(fhd_block_allocator* block_alloc);
void fhd_block_allocator_release(fhd_block_allocator* block_alloc, void* ptr);
void fhd_block_allocator_clear(fhd_block_allocator* block_alloc);
