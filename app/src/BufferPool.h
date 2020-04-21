#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "Pool.h"
#include "sortnet/concepts.h"
#include "sortnet/z_environment.h"

namespace sortnet {
template <concepts::Set Set, concepts::ComparatorNetwork Net, uint64_t size> class BufferSet {
public:
  std::vector<Set> sets{};
  std::vector<Net> nets{};

  constexpr BufferSet() : sets(size), nets(size) {}
};

template <concepts::Set Set, concepts::ComparatorNetwork Net, uint8_t NrOfCores,
          uint64_t bufferSize>
class BufferPool : public Pool<BufferSet<Set, Net, bufferSize>> {};
}  // namespace sortnet