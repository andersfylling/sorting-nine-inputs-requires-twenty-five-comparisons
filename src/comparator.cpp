#include <sortnet/comparator.h>
#include <sortnet/util.h>

void Comparator::write(std::ofstream &f) const {
  binary_write(f, from);
  binary_write(f, to);
}

void Comparator::read(std::ifstream &f) {
  binary_read(f, from);
  binary_read(f, to);
}

template <uint8_t N>
std::string Comparator::to_string() {
  const auto base{N - 1};
  return "(" + std::to_string(int(base - from)) + "," + std::to_string(int(base - to)) + ");";
}
