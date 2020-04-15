#include "sortnet/util.h"

#include <sys/stat.h>

#include <bit>
#include <cmath>
#include <iostream>
#include <sstream>

bool fileExists(const std::string &filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

double FloatPrecision(double v, double p) { return (floor((v * pow(10, p) + 0.5)) / pow(10, p)); }

template <uint8_t N, SeqSet Set>
std::string set_to_string(const Set &set) {
  std::stringstream ss{};
  ss << "{";
  for (auto k{1}; k < N; ++k) {
    ss << "{";
    for (auto it{set.cbegin()}; it != set.cend(); ++it) {
      const sequence_t s{*it};
      if (std::popcount(s) != k) {
        continue;
      }
      ss << ::sequence::binary::to_string<N>(s) << ",";
    }
    ss << "}, ";
  }
  ss << "}";
  return std::string(ss.str());
}