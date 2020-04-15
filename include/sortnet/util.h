#pragma once

#include <array>
#include <fstream>
#include <limits>

#include "../config/config.h"
#include "sequence.h"
#include "sortnet/sets/concept.h"

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

// these nice helper funcs (binary_read and binary_write) were inspired by:
// https://baptiste-wicht.com/posts/2011/06/write-and-read-binary-files-in-c.html
template <typename T>
std::istream &binary_read(std::istream &f, T &value) {
  return f.read(reinterpret_cast<char *>(&value), sizeof(T));
}

template <typename T>
std::ostream &binary_write(std::ostream &f, const T &value) {
  return f.write(reinterpret_cast<const char *>(&value), sizeof(T));
}

bool fileExists(const std::string &filename);

double FloatPrecision(double v, double p);

template <uint8_t N, SeqSet Set>
std::string set_to_string(const Set &set);