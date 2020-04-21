#pragma once

#include <sortnet/bits.h>

#include <array>
#include <bit>
#include <bitset>
#include <string>

#include "z_environment.h"

namespace sortnet {
  namespace sequence::binary {
    using sequence_t = uint_fast16_t;
    template <uint8_t N> constexpr sequence_t genMask() {
      static_assert(N > 1, "N must be above 1");
      sequence_t mask{0};

      for (uint8_t i{0}; i < N; ++i) {
        mask |= sequence_t(1) << i;
      }
      return mask;
    }

    template <uint8_t N> constexpr sequence_t mask{genMask<N>()};

    template <uint8_t N> constexpr std::size_t size() {
      static_assert(N > 1, "N must be above 1");
      return (std::numeric_limits<sequence_t>::max() & mask<N>) - 1;
    }

    // all sequences from 1 until the last integer with N set bits
    template <uint8_t N> auto all{[]() {
      std::array<sequence_t, size<N>()> values{};

      auto i{0};
      for (sequence_t v{1}; v <= size<N>(); ++v) {
        values.at(i) = v;
        ++i;
      }
      return values;
    }()};

    template <uint8_t N> std::string to_string(sequence_t s) {
      auto bs = std::bitset<N>(s);
      return bs.to_string();
    }
  }  // namespace sequence::binary

  // default
  using sequence_t = sequence::binary::sequence_t;
}  // namespace sortnet