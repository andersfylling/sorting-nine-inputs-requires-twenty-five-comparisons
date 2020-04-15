#pragma once

#include "sortnet/sequence.h"

template <class T>
concept SeqSet = requires(T a, T b, uint8_t k, sequence_t s) {
  { a.size() }
  ->std::size_t;
  { a.contains(s) }
  ->bool;
  { a.contains(k, s) }
  ->bool;
  { a.subsumes(b) }
  ->bool;
  { a.computeMeta() }
};
