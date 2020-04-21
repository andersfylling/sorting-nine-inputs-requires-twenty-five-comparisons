#pragma once

#include "sortnet/comparator.h"
#include "sortnet/sequence.h"

namespace sortnet {
namespace concepts {
template <class T> concept ComparatorNetwork = requires(T net, sequence_t s, Comparator c) {
  { net.size() }
  ->std::size_t;
  {net.clear()};
  { net.back() }
  ->Comparator;
  {net.pop_back()};
  {net.push_back(c)};
  { net.run(s) }
  ->sequence_t;
};

template <class T> concept Set = requires(T set, T other, uint8_t k, sequence_t s) {
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
}  // namespace concepts
}  // namespace sortnet