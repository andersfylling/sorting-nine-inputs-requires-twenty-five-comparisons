#pragma once

#include <sortnet/io.h>
#include <sortnet/permutation.h>
#include <sortnet/sequence.h>
#include <sortnet/sets/Metadata.h>
#include <sortnet/z_environment.h>

#include <algorithm>
#include <list>

namespace sortnet::set {
template <uint8_t N, uint8_t K> class ListNaive {
private:
  std::list<sequence_t> seqs{};

public:
  Metadata<N> metadata;

  constexpr ListNaive() = default;
  constexpr ListNaive(const ListNaive &rhs) : metadata(rhs.metadata) {
    std::copy(rhs.seqs.cbegin(), rhs.seqs.cend(), std::back_inserter(seqs));
  };
  constexpr ListNaive &operator=(const ListNaive &rhs) {
    metadata = rhs.metadata;
    seqs.clear();
    std::copy(rhs.seqs.cbegin(), rhs.seqs.cend(), std::back_inserter(seqs));
    return *this;
  };
  constexpr bool operator==(const ListNaive &rhs) {
    if (seqs.size() != rhs.seqs.size()) {
      return false;
    }

    return std::equal(seqs.cbegin(), seqs.cend(), rhs.cbegin());
  }

  constexpr void clear() {
    seqs.clear();
    metadata.clear();
  }

  [[nodiscard]] constexpr std::size_t size() const { return seqs.size(); }

  [[nodiscard]] constexpr bool contains(const sequence_t s) const {
    return std::find(seqs.cbegin(), seqs.cend(), s) != seqs.cend();
  }

  [[nodiscard]] constexpr bool contains([[maybe_unused]] const int8_t k,
                                        const ::sortnet::sequence_t s) const {
    return contains(s);
  }

  // WARNING: you can not insert a binary sequence with N activated bits,
  // as this causes undefined behaviour.
  constexpr void insert(const int8_t k, const ::sortnet::sequence_t s) {
#if (PREFER_SAFETY == 1)
    constexpr ::sortnet::sequence_t mask{::sortnet::sequence::binary::mask<N>};
    const ::sortnet::sequence_t filtered{s & mask};
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
    for (const ::sortnet::sequence_t s : seqs) {
      if (std::find(other.cbegin(), other.cend(), s) == other.cend()) {
        return false;
      }
    }

    return true;
  }

  constexpr void computeMeta() { metadata.compute(); }

  // iterator
  using const_iterator = typename std::list<::sortnet::sequence_t>::const_iterator;
  [[nodiscard]] const_iterator begin() const noexcept { return seqs.cbegin(); }
  [[nodiscard]] const_iterator cbegin() const noexcept { return seqs.cbegin(); }
  [[nodiscard]] const_iterator end() const noexcept { return seqs.cend(); }
  [[nodiscard]] const_iterator cend() const noexcept { return seqs.cend(); }

  // file manipulation
  void write(std::ostream &f) const {
    metadata.write(f);

    int32_t _size{size()};
    ::sortnet::binary_write(f, _size);

    for (const sequence_t s : seqs) {
      ::sortnet::binary_write(f, s);
    }
  }

  void read(std::istream &f) {
    clear();
    metadata.read(f);

    int32_t _size{0};
    ::sortnet::binary_read(f, _size);

    for (int32_t i{0}; i < _size; ++i) {
      sequence_t s{0};
      ::sortnet::binary_read(f, s);
      seqs.push_back(s);
    }
  }
};
}  // namespace sortnet::set