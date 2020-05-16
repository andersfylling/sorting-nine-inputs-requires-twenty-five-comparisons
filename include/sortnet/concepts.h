#pragma once

#include <sortnet/comparator.h>
#include <sortnet/sequence.h>

namespace sortnet::concepts {
template <class T> concept ComparatorNetwork
    = requires(T net, ::sortnet::sequence_t s, ::sortnet::Comparator c) {
  { net.size() }
  ->std::same_as<std::size_t>;
  { net.empty() }
  ->std::same_as<bool>;
  {net.clear()};
  { net.back() }
  ->std::same_as<::sortnet::Comparator>;
  {net.pop_back()};
  {net.push_back(c)};
  { net.run(s) }
  ->std::same_as<::sortnet::sequence_t>;
};

template <class T> concept Set = requires(T set, T other, uint8_t k, ::sortnet::sequence_t s) {
  { set.size() }
  ->std::same_as<std::size_t>;
  { set.contains(k, s) }
  ->std::same_as<bool>;
  { set.contains(s) }
  ->std::same_as<bool>;
  { set.subsumes(other) }
  ->std::same_as<bool>;
  {set.insert(k, s)};
  {set.clear()};
  {set.computeMeta()};
};
}  // namespace sortnet::concepts