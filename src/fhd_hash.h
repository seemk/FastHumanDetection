#pragma once

#include <stdint.h>

struct fhd_fnv1a {
  uint64_t state;
};

fhd_fnv1a fhd_fnv1a_create();
uint64_t fhd_fnv1a_hash(const void* data, int length);
uint64_t fhd_fnv1a_hash(fhd_fnv1a* hash_state, const void* data, int length);
