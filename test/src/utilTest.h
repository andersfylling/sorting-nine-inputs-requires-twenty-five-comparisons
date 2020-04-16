#pragma once

#include <string>

#include <sortnet/permutation.h>
#include <sortnet/sequence.h>
#include <sortnet/util.h>

#include <sortnet/z_environment.h>

template <uint8_t N>
constexpr bool isSorted(const sequence_t s) {
  const auto k{std::popcount(s)};
  return (s >> k) == 0;
}

template <uint8_t N>
constexpr ::permutation::permutation_t<N> perm(const std::array<uint8_t, N> &pArr) {
  return ::permutation::createPaper<N>(pArr);
}

template <uint8_t N>
constexpr Comparator comp(const uint8_t from, const uint8_t to) {
  return Comparator(N-1-from, N-1-to);
}
