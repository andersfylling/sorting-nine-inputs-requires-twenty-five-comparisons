#pragma once

// compile time configurations
// ----------------------------------------
#ifndef UNIT_TEST
#  define UNIT_TEST 0
#endif
// ----------------------------------------
#ifndef RECORD_INTERNAL_METRICS
#  define RECORD_INTERNAL_METRICS 1
#endif
// ----------------------------------------
#ifndef LEMMA_7
#  define LEMMA_7 1
#endif
// ----------------------------------------
#ifndef SAVE_METRICS
#  if (UNIT_TEST == 1)
#    define SAVE_METRICS 0
#  else
#    define SAVE_METRICS 1
#  endif
#endif
// ----------------------------------------
#ifndef RECORD_ANALYSIS
#  if (UNIT_TEST == 1)
#    define RECORD_ANALYSIS 0
#  else
#    define RECORD_ANALYSIS 1
#  endif
#endif
// ----------------------------------------
#ifndef RECORD_IO_TIME
#  define RECORD_IO_TIME 0
#endif
// ----------------------------------------
#ifndef PREFER_SAFETY
#  define PREFER_SAFETY 1
#endif
// ----------------------------------------
#ifndef PRINT_LAYER_SUMMARY
#  if (UNIT_TEST == 1)
#    define PRINT_LAYER_SUMMARY 0
#  else
#    define PRINT_LAYER_SUMMARY 1
#  endif
#endif
// ----------------------------------------
#ifndef PRINT_PROGRESS
#  if (UNIT_TEST == 1)
#    define PRINT_PROGRESS 0
#  else
#    define PRINT_PROGRESS 1
#  endif
#endif
// ----------------------------------------
#ifndef SEGMENT_SIZE
#  define SEGMENT_SIZE 5000
#endif
// ----------------------------------------
#if (PREFER_SAFETY == 0)
//#define at(x) operator[](x)
#endif
// ----------------------------------------

// custom types
#include <cstdint>

namespace sortnet {
  using segmentID_t = uint64_t;

  // custom values
  constexpr uint32_t segment_capacity{SEGMENT_SIZE};
}  // namespace sortnet