#include <sortnet/util.h>

namespace sortnet {

  double FloatPrecision(double v, double p) { return (floor((v * pow(10, p) + 0.5)) / pow(10, p)); }
}  // namespace sortnet