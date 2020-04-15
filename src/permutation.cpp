#include "sortnet/permutation.h"

#include <sstream>

namespace permutation {
template <uint8_t N>
std::string to_string(permutation_t<N> p) {
  std::stringstream ss{};
  ss << "(";
  for (auto i{0}; i < p.size(); ++i) {
    ss << p.at(i);
    if (i + 1 < p.size()) {
      ss << ",";
    }
  }
  ss << ")";

  return ss.str();
}
}  // namespace permutation