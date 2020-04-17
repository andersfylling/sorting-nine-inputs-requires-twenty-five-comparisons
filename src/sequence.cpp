#include <sortnet/sequence.h>

namespace sortnet {
  namespace sequence::binary {
    template <uint8_t N> std::string to_string(const sequence_t s) {
      auto bs = std::bitset<N>(s);
      return bs.to_string();
    }

    template <uint8_t N> std::string to_set_string(const sequence_t s) {
      return "{" + to_string<N>(s) + "}";
    }
  }  // namespace sequence::binary
}