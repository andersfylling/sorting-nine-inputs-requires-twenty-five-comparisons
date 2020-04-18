#pragma once

#include <array>
#include <fstream>
#include <limits>
#include <string>
#include <bit>
#include <iostream>
#include <sstream>
#include <cmath>

#include "sequence.h"
#include "sortnet/concepts.h"
#include "z_environment.h"

namespace sortnet {
constexpr uint64_t factorial(uint64_t n) {
  if (n == 0) {
    return 1;
  }
  return n * factorial(n - 1);
}

template <uint8_t N>
constexpr uint8_t networkSizeUpperBound() {
  static_assert(N > 1, "N must be above 1");
  static_assert(N < 17, "N can not be higher than 17");

  uint8_t margin = 3;
  uint8_t size   = 0;
  switch (N) {
    case 1:
      size = 0;
      break;
    case 2:
      size = 1;
      break;
    case 3:
      size = 3;
      break;
    case 4:
      size = 5;
      break;
    case 5:
      size = 9;
      break;
    case 6:
      size = 12;
      break;
    case 7:
      size = 16;
      break;
    case 8:
      size = 19;
      break;
    case 9:
      size = 25;
      break;
    case 10:
      size = 29;
      break;
    case 11:
      size = 35 + margin * 1;
      break;
    case 12:
      size = 39 + margin * 2;
      break;
    case 13:
      size = 45 + margin * 3;
      break;
    case 14:
      size = 51 + margin * 4;
      break;
    case 15:
      size = 56 + margin * 5;
      break;
    case 16:
      size = 60 + margin * 6;
      break;
    case 17:
      size = 71 + margin * 7;
      break;
    default:
      throw std::logic_error("unsupported number of elements: " + std::to_string(N));
  }

  return size;
}

double FloatPrecision(double v, double p) { return (floor((v * pow(10, p) + 0.5)) / pow(10, p)); }

  // string representation of a set with ordered partitions
  template <uint8_t N, concepts::Set set_t>
  std::string to_string(set_t& set) {
    std::stringstream ss{};
    ss << "(";
    for (auto k{0}; k < N-1; ++k) {
      ss << "{";
      for (const sequence_t s : set) {
        if (std::popcount(s)-1 != k) {
          continue;
        }
        ss << sequence::binary::to_string<N>(s) << ",";
      }
      ss.seekp(-1, std::ios_base::end);
      ss << "},";
    }
    ss.seekp(-1, std::ios_base::end);
    ss << ")";

    return std::string(ss.str());
  }

  // same as in papers => (from, to); (from, to); etc.
  std::string to_string(const Comparator c, const uint8_t N) {
    const auto base{N - 1};
    return "(" + ::std::to_string(int(base - c.from)) + "," + ::std::to_string(int(base - c.to)) + ");";
  }

  template <concepts::ComparatorNetwork net_t>
  std::string to_string_knuth_diagram(const net_t& net, const uint8_t N, const sequence_t s = 0) {
    sequence_t sOutput{};
    if (s > 0) {
      sOutput = net.run(s);
    }
    std::string output{};

    for (auto i = 0; i < N; i++) {
      auto ir = (N - 1) - i;
      // vertex / node
      output += s == 0 ? " " : std::to_string((s >> ir) & 1);
      for (auto it{net.cbegin()}; it != net.cend(); ++it) {
        const Comparator& c{*it};
        output += "--";
        if (c.from == i || c.to == i) {  // TODO: might need to be inverted!
          output += "+";
        } else {
          output += "-";
        }
      }
      output += "--";
      output += s == 0 ? " " : std::to_string((sOutput >> ir) & 1);
      output += "\n";

      if (i + 1 == N) {
        break;
      }

      // spaces & edge
      output += " ";
      for (auto it{net.cbegin()}; it != net.cend(); ++it) {
        const Comparator& c{*it};
        output += "  ";
        if (i < c.to && (i == c.from || i > c.from)) {  // TODO: might need to be inverted!
          output += "|";
        } else {
          output += " ";
        }
      }
      output += "\n";
    }

    return output;
  }

  template <concepts::ComparatorNetwork net_t>
  std::string to_string(const net_t& net, uint8_t N, sequence_t s = 0) {
    sequence_t sOutput{};
    if (s > 0) {
      sOutput = net.run(s);
    }
    std::string output{};

    // by default networks as displayed as a graphically using +, | and -.
    // the compact version simply list the comparators as ordered sets
    // with node numbers in a left to right, top-down traversal.
    // eg: "(1, 2); (3, 4); (1, 4);"
    std::size_t i{0};
    for (auto it{net.cbegin()}; it != net.cend(); ++it) {
      const Comparator& c{*it};
      if (i == net.size()) {
        break;
      }
      output += to_string(c, N) + " ";
      ++i;
    }

    if (sOutput > 0) {
      // TODO: fixed size / zeros padding
      output += "\n" + std::to_string(s) + " => " + std::to_string(sOutput);
    }

    return output + "\n";
  }
}