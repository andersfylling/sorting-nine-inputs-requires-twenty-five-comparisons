#pragma once

#include <sortnet/sets/Metadata.h>
#include <sortnet/z_environment.h>

#include <algorithm>
#include <list>

#include "sortnet/permutation.h"
#include "sortnet/sequence.h"

namespace sortnet {
  namespace set {
    template <uint8_t N, uint8_t K> class ListNaive {
    private:
      std::list<sequence_t> seqs{};

    public:
      Metadata<N> metadata;

      constexpr ListNaive() = default;
      constexpr ListNaive(const ListNaive &rhs) : metadata(rhs.metadata) {
        // do not use =default as this copies a std::list reference
        std::copy(rhs.seqs.cbegin(), rhs.seqs.cend(), seqs.begin());
      };
      constexpr ListNaive &operator=(const ListNaive &rhs) {
        metadata = rhs.metadata;
        std::copy(rhs.seqs.cbegin(), rhs.seqs.cend(), seqs.begin());
      };
      constexpr bool operator==(const ListNaive &rhs) {
        if (seqs.size() != rhs.seqs.size()) {
          return false;
        }

        return std::equal(seqs.cbegin(), seqs.cend(), rhs.cbegin());
      }

      // deprecated
      constexpr void reset() { clear(); }

      constexpr void clear() {
        seqs.clear();
        metadata.clear();
      }

      constexpr std::size_t size() const { return seqs.size(); }

      constexpr bool contains(const sequence_t s) const {
        return std::find(seqs.cbegin(), seqs.cend(), s) != seqs.cend();
      }

      constexpr bool contains([[maybe_unused]] const int8_t k, const sequence_t s) const {
        return contains(s);
      }

      // WARNING: you can not insert a binary sequence with N activated bits,
      // as this causes undefined behaviour.
      constexpr void insert(const int8_t k, const sequence_t s) {
#if (PREFER_SAFETY == 1)
        constexpr sequence_t mask{sequence::binary::mask<N>};
        const sequence_t filtered{s & mask};
        if (filtered == mask || filtered == 0) {
          return;
        }
#endif
        if (contains(k, s)) {
          return;
        }

        seqs.push_back(s);
        metadata.compute(k, s);
      }

      bool subsumes(const ListNaive &other) const {
        for (const sequence_t s : seqs) {
          if (std::find(other.cbegin(), other.cend(), s) == other.cend()) {
            return false;
          }
        }

        return true;
      }

      constexpr void computeMeta() { metadata.compute(); }

      // iterator
      using const_iterator = typename std::list<sequence_t>::const_iterator;
      using iterator = typename std::list<sequence_t>::iterator;
      iterator begin() { return seqs.begin(); }
      [[nodiscard]] const_iterator cbegin() const noexcept { return seqs.cbegin(); }
      iterator end() { return seqs.end(); }
      [[nodiscard]] const_iterator cend() const noexcept { return seqs.cend(); }

      // file manipulation
      void write(std::ostream &f) const {
        metadata.write(f);

        std::size_t _size{size()};
        binary_write(f, _size);

        for (const auto s : seqs) {
          binary_write(f, s);
        }
      }

      void read(std::istream &f) {
        reset();
        metadata.read(f);

        std::size_t _size{0};
        binary_read(f, _size);

        for (std::size_t i{0}; i < _size; ++i) {
          sequence_t s{0};
          binary_read(f, s);
          seqs.push_back(s);
        }
      }
    };
  }  // namespace set
}  // namespace sortnet