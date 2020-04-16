#pragma once

#include <array>
#include <bit>

#include "sortnet/sequence.h"

#include "../z_environment.h"

template<uint8_t N>
class Metadata {
 public:
  static const uint8_t size{N-1};

  uint64_t netID;
  bool     marked;

  std::array<sequence_t, size> ones;
  decltype(ones)               zeros;

  std::array<uint8_t, size> onesCount;
  decltype(onesCount)       zerosCount;

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
      onesCount.at(i)  = std::popcount(ones.at(i));
    }
    for (auto i{0}; i < size; ++i) {
      zerosCount.at(i) = std::popcount(zeros.at(i));
    }
  }

  // file manipulation
  void write(std::ostream &f) const;
  void read(std::istream &f);
};
