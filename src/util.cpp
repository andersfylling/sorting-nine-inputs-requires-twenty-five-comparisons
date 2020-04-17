#include <sortnet/util.h>

#include <string>
#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

namespace sortnet {
double FloatPrecision(double v, double p) { return (floor((v * pow(10, p) + 0.5)) / pow(10, p)); }

template <uint8_t N, concepts::Set set_t>
std::string to_string(set_t& set) {
  std::stringstream ss{};
  ss << "(";
  for (auto k{0}; k < N-1; ++k) {
    ss << "{";
    for (const sequence_t s : set) {
      if (std::popcount(s)-1 != k) {
        continue;
      }
      ss << sequence::binary::to_string<N>(s) << ",";
    }
    ss.seekp(-1, std::ios_base::end);
    ss << "},";
  }
  ss.seekp(-1, std::ios_base::end);
  ss << ")";

  return std::string(ss.str());
}
}