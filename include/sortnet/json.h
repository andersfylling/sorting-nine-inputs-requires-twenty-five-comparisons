#pragma once

#include "z_environment.h"

#ifdef at
#define at_removed 1
#undef at
#endif

#include <nlohmann/json.hpp>

#ifdef at_removed
#pragma pop_macro("at")
#endif