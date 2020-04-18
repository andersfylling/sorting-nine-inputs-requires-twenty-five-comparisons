#include <sortnet/util.h>

namespace sortnet{
  double FloatPrecision(double v, double p) {
    return (floor((v * pow(10, p) + 0.5)) / pow(10, p));
  }

  std::string to_string(const Comparator c, const uint8_t N) {
    const auto base{N - 1};
    return "(" + ::std::to_string(int(base - c.from)) + "," + ::std::to_string(int(base - c.to))
           + ");";
  }
}