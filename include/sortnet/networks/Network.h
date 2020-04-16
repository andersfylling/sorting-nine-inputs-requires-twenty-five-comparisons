#pragma once

#include <vector>

#include "sortnet/comparator.h"
#include "sortnet/util.h"

#include "../z_environment.h"

template <uint8_t N, uint8_t K>
class Network {
 private:
  std::vector<Comparator> comparators{};

  constexpr void addUnsafe(const Comparator c) {
    comparators.push_back(c);
  }

  constexpr void addSafe(const Comparator c) {
    if (comparators.size() == K) {
      throw std::logic_error("this network can not fit any more comparators");
    }

    auto pos = ::comparator::pos<N>(c);
    if (c == 0 || pos.first > N || pos.second > N || pos.first <= pos.second) {
      throw std::logic_error("comparator is invalid according to spec");
    }
    addUnsafe(c);
  }

 public:
  uint64_t id;

  constexpr Network() = default;
  constexpr Network(const Network &rhs) = default;
  constexpr Network &operator=(const Network &rhs) = default;

  [[nodiscard]] constexpr std::size_t size() const { return comparators.size(); }

  constexpr void clear() {
    id    = 0;
    comparators.clear();
  }

  constexpr void         pop_back() { comparators.pop_back(); }
  constexpr Comparator back() {
    if (comparators.empty()) {
      return Comparator(0,0);
    }
    return comparators.back();
  }

  constexpr void add(const Comparator c) {
#if (PREFER_SAFETY == 1)
    addSafe(c);
#else
    addUnsafe(c);
#endif
  }

  [[nodiscard]] constexpr sequence_t run(sequence_t s) const {
    for (const auto& c : comparators) {
      s = c.apply(s);
    }
    return s;
  };

  void                      write(std::ofstream &f) const;
  void                      read(std::ifstream &f);
  [[nodiscard]] std::string to_string(sequence_t s = 0) const;
  [[nodiscard]] std::string knuthDiagram(sequence_t s = 0) const;
};
