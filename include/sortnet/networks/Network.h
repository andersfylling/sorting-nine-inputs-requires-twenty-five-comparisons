#pragma once

#include <vector>
#include <string>

#include "sortnet/comparator.h"

#include "../z_environment.h"

namespace sortnet {
  namespace network {
    template <uint8_t N, uint8_t K> class Network {
    private:
      std::vector<Comparator> comparators{};

      constexpr void addUnsafe(const Comparator c) { comparators.push_back(c); }

      constexpr void addSafe(const Comparator c) {
        if (comparators.size() == K) {
          throw std::logic_error("this network can not fit any more comparators");
        }

        if (c.empty() || !(c.from >= 0 && c.from < N) || !(c.to >= 0 && c.to < N) || c.from <= c.to) {
          throw std::logic_error("comparator is invalid according to spec");
        }
        addUnsafe(c);
      }

    public:
      uint64_t id;

      constexpr Network() = default;
      constexpr Network(const Network &rhs) = default;
      constexpr Network &operator=(const Network &rhs) = default;
      constexpr bool operator==(const Network &rhs) const {
        if (size() != rhs.size()) {
          return false;
        }
        return std::equal(comparators.cbegin(), comparators.cend(), rhs.comparators.cbegin());
      };

      [[nodiscard]] constexpr std::size_t size() const { return comparators.size(); }

      constexpr void clear() {
        id = 0;
        comparators.clear();
      }

      constexpr void pop_back() { comparators.pop_back(); }
      constexpr Comparator back() {
        if (comparators.empty()) {
          return Comparator(0, 0);
        }
        return comparators.back();
      }

      constexpr void push_back(const Comparator c) {
#if (PREFER_SAFETY == 1)
        addSafe(c);
#else
        addUnsafe(c);
#endif
      }

      [[nodiscard]] constexpr sequence_t run(sequence_t s) const {
        for (const auto &c : comparators) {
          s = c.apply(s);
        }
        return s;
      };

      void write(std::ofstream &f) const;
      void read(std::ifstream &f);
      [[nodiscard]] std::string to_string(sequence_t s = 0) const;
      [[nodiscard]] std::string knuthDiagram(sequence_t s = 0) const;
    };
  }
  }