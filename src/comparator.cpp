#include <sortnet/comparator.h>

namespace sortnet {
void Comparator::write(std::ostream &f) const {
  binary_write(f, from);
  binary_write(f, to);
}

void Comparator::read(std::istream &f) {
  binary_read(f, from);
  binary_read(f, to);
}
}  // namespace sortnet
