#include "fhd_perf.h"

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
uint64_t fhd_rdtsc() { return __rdtsc(); }
