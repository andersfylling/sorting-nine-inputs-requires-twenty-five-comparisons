#pragma once

#include <sortnet/z_environment.h>

#include <array>
#include <bit>

#include "sortnet/io.h"
#include "sortnet/sequence.h"

namespace sortnet::set {
template <uint8_t N> class Metadata {
public:
  static const uint8_t size{N - 1};

  uint64_t netID{0};
  bool marked{false};

  std::array<sequence_t, size> ones;
  decltype(ones) zeros;

  std::array<uint8_t, size> onesCount;
  decltype(onesCount) zerosCount;

  // if the output set has a size larger than 65,535 the uint
  // must be changed to reflect so.
  // std::numeric_limits<uint16_t>::max() == 65,535
  std::array<uint16_t, size> sizes;

  constexpr Metadata() = default;
  constexpr Metadata(const Metadata &rhs) = default;
  constexpr Metadata &operator=(const Metadata &rhs) = default;

  constexpr void clear() {
    marked = false;
    netID = 0;
    std::fill(ones.begin(), ones.end(), 0);
    std::fill(onesCount.begin(), onesCount.end(), 0);
    std::fill(zeros.begin(), zeros.end(), 0);
    std::fill(zerosCount.begin(), zerosCount.end(), 0);
    std::fill(sizes.begin(), sizes.end(), 0);
  }

  constexpr void compute(const uint8_t k, const sequence_t s) {
    sizes.at(k)++;
    ones.at(k) |= s;
    zeros.at(k) |= ~s;
  }

  constexpr void compute() {
    // cache popcount of ones and zeros
    for (auto i{0}; i < size; ++i) {
      onesCount.at(i) = std::popcount(ones.at(i));
    }
    for (auto i{0}; i < size; ++i) {
      zerosCount.at(i) = std::popcount(zeros.at(i));
    }
  }

  // serialize
  void write(std::ostream &f) const {
    ::sortnet::binary_write(f, netID);
    ::sortnet::binary_write(f, ones);
    ::sortnet::binary_write(f, onesCount);
    ::sortnet::binary_write(f, zeros);
    ::sortnet::binary_write(f, zerosCount);
    ::sortnet::binary_write(f, sizes);
  }

  void read(std::istream &f) {
    marked = false;

    ::sortnet::binary_read(f, netID);
    ::sortnet::binary_read(f, ones);
    ::sortnet::binary_read(f, onesCount);
    ::sortnet::binary_read(f, zeros);
    ::sortnet::binary_read(f, zerosCount);
    ::sortnet::binary_read(f, sizes);
  }
};

}  // namespace sortnet::set