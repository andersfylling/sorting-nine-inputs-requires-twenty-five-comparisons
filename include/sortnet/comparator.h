#pragma once

#include <sortnet/io.h>
#include <sortnet/sequence.h>
#include <sortnet/z_environment.h>

#include <array>
#include <string>

namespace sortnet {
  class Comparator {
  private:
  public:
    uint8_t from;
    uint8_t to;

    constexpr Comparator() : from(0), to(0){};
    constexpr Comparator(const uint8_t from, const uint8_t to) : from(from), to(to){};
    constexpr Comparator(const Comparator &rhs) = default;
    constexpr Comparator &operator=(const Comparator &rhs) = default;
    constexpr bool operator==(const Comparator &rhs) const {
      return rhs.from == from && rhs.to == to;
    };

    constexpr bool empty() const { return from == 0 && to == 0; }

    [[nodiscard]] constexpr sequence_t apply(const sequence_t s) const {
      sequence_t pattern = uint8_t(1) << from;
      sequence_t seqMask = pattern | sequence_t(uint8_t(1) << to);
      return (seqMask & s) == pattern ? sequence_t(s | seqMask) ^ pattern : s;
    }

    // serialize
    void write(std::ostream &stream) const;
    void read(std::istream &stream);
  };

  namespace comparator {
    template <uint8_t N> constexpr Comparator _paper_create(uint8_t from, uint8_t to) {
      // needs to be reversed as a network 0 position
      // does not equal a binary sequence 0 position.
      constexpr uint8_t bottom{N - 1};
      return Comparator(bottom - from, bottom - to);
    }

    template <uint8_t N> constexpr std::size_t size() {
      static_assert(N > 1, "N must be above 1");

      std::size_t counter = 0;
      for (uint8_t i = 0; i < N; ++i) {
        for (uint8_t j = i + 1; j < N; ++j) {
          ++counter;
        }
      }

      return counter;
    }

    template <uint8_t N> constexpr auto all{[]() {
      std::array<::sortnet::Comparator, size<N>()> comparators{};

      std::size_t i{0};
      for (int from{N - 1}; from != 0; --from) {
        for (int to{from - 1}; to >= 0; --to) {
          comparators.at(i)
              = ::sortnet::Comparator(static_cast<uint8_t>(from), static_cast<uint8_t>(to));
          ++i;
        }
      }

      return comparators;
    }()};
  }  // namespace comparator
}  // namespace sortnet