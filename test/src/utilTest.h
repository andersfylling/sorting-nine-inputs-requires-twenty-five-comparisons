#pragma once

#include <sortnet/concepts.h>
#include <sortnet/permutation.h>
#include <sortnet/sequence.h>
#include <sortnet/z_environment.h>

#include <string>

template <uint8_t N, ::sortnet::concepts::ComparatorNetwork net_t, ::sortnet::concepts::Set set_t>
constexpr void populate(const net_t& net, set_t& set) {
  constexpr ::sortnet::sequence_t mask{::sortnet::sequence::binary::genMask<N>()};
  constexpr auto max = std::numeric_limits<::sortnet::sequence_t>::max() & mask;
  for (::sortnet::sequence_t s{1}; s < max; s++) {
    const auto k{std::popcount(s) - 1};
    set.insert(k, net.run(s));
  }
}

template <uint8_t N> constexpr bool isSorted(const ::sortnet::sequence_t s) {
  const auto k{std::popcount(s)};
  return (s >> k) == 0;
}

template <uint8_t N>
constexpr ::sortnet::permutation::permutation_t<N> perm(const std::array<uint8_t, N>& pArr) {
  return ::sortnet::permutation::createPaper<N>(pArr);
}

template <uint8_t N> constexpr ::sortnet::Comparator comp(const uint8_t from, const uint8_t to) {
  return ::sortnet::Comparator(N - 1 - from, N - 1 - to);
}
