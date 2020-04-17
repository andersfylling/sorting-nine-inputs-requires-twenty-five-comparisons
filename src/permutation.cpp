#include "sortnet/permutation.h"

#include <sstream>

namespace sortnet {
  namespace permutation {
    template <uint8_t N> std::string to_string(permutation_t<N> p) {
      std::stringstream ss{};
      ss << "(";
      for (int8_t i{p.size()-1}; i >= 0; --i) {
        ss << int(N - 1 - p.at(i)); // reverse to be paper compliant
        if (i - 1 >= 0) {
          ss << ",";
        }
      }
      ss << ")";

      return ss.str();
    }
  }  // namespace permutation
}