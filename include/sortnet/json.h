#pragma once

#ifdef at
#define at_removed 1
#undef at
#endif

#include "sortnet/vendors/github.com/nlohmann/json/json.hpp"

#ifdef at_removed
#pragma pop_macro("at")
#endif