#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "../config/config.h"
#include "BoolArray.h"
#include "Pool.h"
#include "sortnet/networks/concept.h"
#include "sortnet/sets/concept.h"

template<SeqSet Set, ComparatorNetwork Net, uint64_t size>
class BufferSet {
 public:
  uint64_t netID{0};
  BoolArray<size> positions{};

  std::vector<Set> setsFirst{};
  std::vector<Net> netsFirst{};
  std::vector<Set> setsSecond{};
  std::vector<Net> netsSecond{};

  constexpr BufferSet() : setsFirst(size), netsFirst(size), setsSecond(size), netsSecond(size) {}

  constexpr void clear() {
    netID = 0;
    positions.clear();
  }
};

template<SeqSet Set, ComparatorNetwork Net, uint8_t NrOfCores, uint64_t bufferSize>
class BufferPool : public Pool<BufferSet<Set, Net, bufferSize>>{
 public:
  void clearBuffers() {
    for (auto* buffer : this->objects) {
      buffer->clear();
    }
  }

  template <typename _II, typename _II2>
  void markPositions(_II it, const _II2 end) {
    BoolArray<bufferSize> positions{};
    for (auto* buffer : this->objects) {
      positions |= buffer->positions;
    }

    const auto begin = it;
    for (; it != end; ++it) {
      const auto position{std::distance(begin, it)};
      it->metadata.marked = positions.at(position);
    }
  }
};