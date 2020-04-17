#pragma once

#include <array>
#include <bitset>
#include <bit>
#include <string>

#include <sortnet/bits.h>

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
      auto max = std::numeric_limits<sequence_t>::max() & mask<N>;

      // remove max value (which is only 1's => 0b1111111)
      // remember we're returning the max value here, not the number of elements.
      // so while it seems more obvious to return max-2 (to remove 0 and 1's)
      // it won't reflect the number of total sequences.
      return max - 1;
    }

    // all sequences from 1 until the last integer with N set bits
    template <uint8_t N> auto all{[]() -> std::array<sequence_t, size<N>()> {
      auto max = std::numeric_limits<sequence_t>::max() & mask<N>;
      std::array<sequence_t, size<N>()> values{};

      auto i{0};
      for (sequence_t v{1}; v < max; ++v) {
        values.at(i) = v;
        ++i;
      }
      return values;
    }};

    // the size of each cluster, where each index maps to the number of set bits
    // subtract one. eg. for the set 0b001, the number of bits is 1 and we subtract
    // one to get the index of 0.
    //  0b001 => 0
    //  0b101 => 1
    //  0b000 => panic, we never deal with sequences without set bits
    template <uint8_t N> auto clusterSizes{[]() -> std::array<std::size_t, N - 1> {
      std::array<sequence_t, N - 1> sizes{};

      for (const auto s : all<N>) {
        const auto k{std::popcount(s) - 1};
        sizes.at(k)++;
      }
      return sizes;
    }};

    // all sequences from 1 until the last integer with N set bits
    template <uint8_t N> auto allClustered{[]() -> std::array<sequence_t, size<N>()> {
      auto max = std::numeric_limits<sequence_t>::max() & mask<N>;
      std::array<sequence_t, size<N>()> values{};

      auto i{0};
      for (sequence_t v{1}; v < max; ++v) {
        values.at(i) = v;
        ++i;
      }
      return values;

      std::array<sequence_t, N - 1> sizes{};

      for (const auto s : all<N>) {
        const auto k{std::popcount(s) - 1};
        values.at() = s;
        sizes.at(k)++;
      }
      return sizes;
    }};

    template <uint8_t N> std::string to_string(sequence_t s);
    template <uint8_t N> std::string to_set_string(sequence_t s);
  }  // namespace sequence::binary

  // default
  using sequence_t = sequence::binary::sequence_t;
}