#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "z_environment.h"
#include "Pool.h"
#include "sortnet/networks/concept.h"
#include "sortnet/sets/concept.h"

template<SeqSet Set, ComparatorNetwork Net, uint64_t size>
class BufferSet {
 public:
  uint64_t netID{0};

  std::vector<Set> setsFirst{};
  std::vector<Net> netsFirst{};
  std::vector<Set> setsSecond{};
  std::vector<Net> netsSecond{};

  constexpr BufferSet() : setsFirst(size), netsFirst(size), setsSecond(size), netsSecond(size) {}

  constexpr void clear() {
    netID = 0;
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
};