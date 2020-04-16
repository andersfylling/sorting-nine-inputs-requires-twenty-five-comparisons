#pragma once

#include "sortnet/sequence.h"

template <class T>
concept ComparatorNetwork = requires(T net, sequence_t s) {
  { net.size() }
  ->std::size_t;
  {net.clear()};
  {net.undo()};
  { net.run(s) }
  ->sequence_t;
};