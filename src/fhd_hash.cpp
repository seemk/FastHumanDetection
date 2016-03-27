#include "fhd_hash.h"

fhd_fnv1a fhd_fnv1a_create() {
  fhd_fnv1a s;
  s.state = 14695981039346656037u;
  return s;
}

uint64_t fhd_fnv1a_hash(fhd_fnv1a* hash_state, const void* data, int length) {
  uint8_t* begin = (uint8_t*)data;
  uint8_t* end = begin + length;
  for (; begin < end; begin++) {
    hash_state->state = (hash_state->state ^ *begin) * 1099511628211u;
  }

  return hash_state->state;
}

uint64_t fhd_fnv1a_hash(const void* data, int length) {
  fhd_fnv1a s = fhd_fnv1a_create();
  return fhd_fnv1a_hash(&s, data, length);
}
