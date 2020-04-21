#pragma once

#include <array>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "sequence.h"
#include "sortnet/concepts.h"
#include "z_environment.h"

namespace sortnet::permutation {
// index represents the bit position from right to left
// eg. index 0 represents the least significant bit
// 0 => 0b011010[0]
// 2 => 0b0110[1]00
//
// for partial permutations the end is symbolized with -1.
template <uint8_t N> using permutation_t = std::array<int8_t, N>;

template <uint8_t N> using constraints_t = std::array<sequence_t, N>;

template <uint8_t N> constexpr void clear(constraints_t<N> &constraints) {
  constexpr sequence_t noConstraints{sequence::binary::genMask<N>()};
  std::fill(constraints.begin(), constraints.end(), noConstraints);
}

template <uint8_t N> constexpr bool valid_fast(const constraints_t<N> &constraints) {
  for (const auto possibility : constraints) {
    if (possibility == 0) {
      return false;
    }
  }

  return true;
}

template <uint8_t N> permutation_t<N> createPaper(const std::array<uint8_t, N> &pArr) {
  permutation_t<N> p{};
  for (auto i{0}; i < N; ++i) {
    p.at(N - 1 - i) = N - 1 - pArr.at(i);
  }

  return p;
}

template <uint8_t N> constexpr sequence_t apply(const permutation_t<N> &p, const sequence_t s) {
  sequence_t product{0};
  for (std::size_t i{0}; i < p.size(); ++i) {
    const auto v{(s >> i) & 1};
    const auto pos{p.at(i)};
    product |= v << pos;
  }
  return product;
}

template <uint8_t N, ::sortnet::concepts::Set set_t>
constexpr void apply(const permutation_t<N> &p, set_t &set) {
  set_t sequences(set);
  set.clear();
  for (const sequence_t s : sequences) {
    const sequence_t permuted = apply<N>(p, s);
    const uint8_t k = std::popcount(permuted - 1);
    set.insert(k, permuted);
  }
}

template <uint8_t N> bool generate(const std::array<sequence_t, N> &constraints,
                                   const std::function<bool(permutation_t<N> &)> &__f) {
  std::vector<std::vector<uint8_t>> stack{};
  permutation_t<N> p{};

  // add the possibilities for the first position
  for (uint8_t i{0}; i < N; ++i) {
    if (((constraints.at(0) >> i) & 0b1) == 1) {
      stack.push_back({i});
    }
  }

  // backtrack permutations from "perfect matching"
  while (!stack.empty()) {
    const auto w{stack.at(stack.size() - 1)};
    stack.pop_back();

    if (w.size() == N) {
      std::copy(w.cbegin(), w.cend(), p.begin());
      if (__f(p)) {
        return true;
      }
      continue;
    }

    // add next constraints
    const auto constraint{constraints.at(w.size())};
    for (uint8_t i{0}; i < N; ++i) {
      if (((constraint >> i) & 0b1) == 0) {
        continue;
      }
      if (std::find(w.cbegin(), w.cend(), i) != w.cend()) {
        continue;
      }

      auto w2(w);
      w2.push_back(i);
      stack.push_back(w2);
    }
  }

  return false;
}

template <uint8_t N> constexpr void constraints(constraints_t<N> &constraints,
                                                const sequence_t base, const sequence_t goal) {
  for (uint8_t i{0}; i < N; ++i) {
    if (((base >> i) & 0b1) == 0) {
      continue;
    }
    constraints.at(i) &= goal;
  }
}

template <uint8_t N, ::sortnet::concepts::Set Set>
constexpr void constraints(constraints_t<N> &c, const Set &setA, const Set &setB) {
  const auto &a{setA.metadata};
  const auto &b{setB.metadata};

  for (auto k{0}; k < a.size; ++k) {
    constraints<N>(c, a.ones.at(k), b.ones.at(k));
    constraints<N>(c, a.zeros.at(k), b.zeros.at(k));
  }
}

template <::sortnet::concepts::Set Set> constexpr bool ST1(const Set &setA, const Set &setB) {
  return setA.size() <= setB.size();
}

template <::sortnet::concepts::Set Set> constexpr bool ST2(const Set &setA, const Set &setB) {
  const auto &a{setA.metadata};
  const auto &b{setB.metadata};

  for (auto i{0}; i < a.size; ++i) {
    if (a.sizes.at(i) > b.sizes.at(i)) {
      return false;
    }
  }

  return true;
}

template <::sortnet::concepts::Set Set> constexpr bool ST3(const Set &setA, const Set &setB) {
  const auto &a{setA.metadata};
  const auto &b{setB.metadata};

  for (auto i{0}; i < a.size; ++i) {
    if (a.onesCount.at(i) > b.onesCount.at(i)) {
      return false;
    }
    if (a.zerosCount.at(i) > b.zerosCount.at(i)) {
      return false;
    }
  }

  return true;
}

template <uint8_t N, ::sortnet::concepts::Set Set>
constexpr bool subsumes(const permutation_t<N> &p, const Set &setA, const Set &setB) {
  for (const sequence_t s : setA) {
    const auto permuted{permutation::apply<N>(p, s)};
    if (!setB.contains(permuted)) {  // O(setB.size())
      return false;
    }
  }
  return true;
}

template <uint8_t N> std::string to_string(permutation_t<N> p) {
  std::stringstream ss{};
  ss << "(";
  for (int8_t i{p.size() - 1}; i >= 0; --i) {
    ss << int(N - 1 - p.at(i));  // reverse to be paper compliant
    if (i - 1 >= 0) {
      ss << ",";
    }
  }
  ss << ")";

  return ss.str();
}
}  // namespace sortnet::permutation