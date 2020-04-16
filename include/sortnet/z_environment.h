#pragma once

// compile time configurations
#ifndef RECORD_INTERNAL_METRICS
#define RECORD_INTERNAL_METRICS 1
#endif
// ----------------------------------------
#ifndef LEMMA_7
#define LEMMA_7 1
#endif
// ----------------------------------------
#ifndef SAVE_METRICS
#define SAVE_METRICS 1
#endif
// ----------------------------------------
#ifndef RECORD_ANALYSIS
#define RECORD_ANALYSIS 1
#endif
// ----------------------------------------
#ifndef RECORD_IO_TIME
#define RECORD_IO_TIME 0
#endif
// ----------------------------------------
#ifndef PREFER_SAFETY
#define PREFER_SAFETY 1
#endif
// ----------------------------------------
#ifndef PRINT_LAYER_SUMMARY
#define PRINT_LAYER_SUMMARY 1
#endif
// ----------------------------------------
#ifndef PRINT_PROGRESS
#define PRINT_PROGRESS 1
#endif
// ----------------------------------------
#ifndef SEGMENT_SIZE
#define SEGMENT_SIZE 5000
#endif
// ----------------------------------------
#if (PREFER_SAFETY == 0)
//#define at(x) operator[](x)
#endif
// ----------------------------------------

// custom types
#include <cstdint>

using segmentID_t = uint64_t;

// custom values
constexpr uint32_t segment_capacity{SEGMENT_SIZE};