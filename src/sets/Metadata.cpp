#include <sortnet/sets/Metadata.h>
#include <sortnet/io.h> // binary_write/binary_read

namespace sortnet {
  namespace set {
    template <uint8_t N> void Metadata<N>::write(std::ostream &f) const {
      binary_write(f, netID);
      binary_write(f, ones);
      binary_write(f, onesCount);
      binary_write(f, zeros);
      binary_write(f, zerosCount);
      binary_write(f, sizes);
    }

    template <uint8_t N> void Metadata<N>::read(std::istream &f) {
      marked = false;

      binary_read(f, netID);
      binary_read(f, ones);
      binary_read(f, onesCount);
      binary_read(f, zeros);
      binary_read(f, zerosCount);
      binary_read(f, sizes);
    }
  }
  }