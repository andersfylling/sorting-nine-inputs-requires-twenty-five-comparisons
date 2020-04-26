#pragma once

#include "sortnet/comparator.h"
#include "sortnet/sequence.h"

namespace sortnet::concepts {
template <class T> concept ComparatorNetwork
    = requires(T net, ::sortnet::sequence_t s, ::sortnet::Comparator c) {
  { net.size() }
  ->std::size_t;
  { net.empty() }
  ->bool;
  {net.clear()};
  { net.back() }
  ->::sortnet::Comparator;
  {net.pop_back()};
  {net.push_back(c)};
  { net.run(s) }
  ->::sortnet::sequence_t;
};

template <class T> concept Set = requires(T set, T other, uint8_t k, ::sortnet::sequence_t s) {
  { set.size() }
  ->std::size_t;
  { set.contains(k, s) }
  ->bool;
  { set.contains(s) }
  ->bool;
  { set.subsumes(other) }
  ->bool;
  {set.insert(k, s)};
  {set.clear()};
  {set.computeMeta()};
};
}  // namespace sortnet::concepts