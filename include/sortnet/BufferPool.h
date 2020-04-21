#pragma once

#include <mutex>
#include <queue>
#include <vector>

#include "Pool.h"
#include "sortnet/concepts.h"
#include "z_environment.h"

namespace sortnet {
  template <concepts::Set Set, concepts::ComparatorNetwork Net, uint64_t size> class BufferSet {
  public:
    uint64_t netID{0};

    std::vector<Set> setsFirst{};
    std::vector<Net> netsFirst{};
    std::vector<Set> setsSecond{};
    std::vector<Net> netsSecond{};

    constexpr BufferSet() : setsFirst(size), netsFirst(size), setsSecond(size), netsSecond(size) {}

    constexpr void clear() { netID = 0; }
  };

  template <concepts::Set Set, concepts::ComparatorNetwork Net, uint8_t NrOfCores,
            uint64_t bufferSize>
  class BufferPool : public Pool<BufferSet<Set, Net, bufferSize>> {
  public:
    void clearBuffers() {
      for (auto* buffer : this->objects) {
        buffer->clear();
      }
    }
  };
}  // namespace sortnet