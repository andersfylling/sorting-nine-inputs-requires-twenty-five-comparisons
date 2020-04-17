#include <sortnet/sets/ListNaive.h>

#include <sortnet/io.h>

namespace sortnet {
  namespace set {
    template <uint8_t N, uint8_t K> void ListNaive<N, K>::write(std::ofstream &f) const {
      metadata.write(f);

      std::size_t _size{size()};
      binary_write(f, _size);

      for (const auto s : seqs) {
        binary_write(f, s);
      }
    }

    template <uint8_t N, uint8_t K> void ListNaive<N, K>::read(std::ifstream &f) {
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
  }
  }