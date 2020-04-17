#include <sortnet/networks/Network.h>
#include <sortnet/io.h>

namespace sortnet {
  namespace network {
    template <uint8_t N, uint8_t K> void Network<N, K>::write(std::ofstream &f) const {
      binary_write(f, id);

      std::size_t _size{comparators.size()};
      binary_write(f, _size);

      for (std::size_t i = 0; i < _size; i++) {
        const Comparator c{comparators.at(i)};
        binary_write(f, c);
      }
    }

    template <uint8_t N, uint8_t K> void Network<N, K>::read(std::ifstream &f) {
      clear();

      binary_read(f, id);

      std::size_t _size{0};
      binary_read(f, _size);

      for (std::size_t i{0}; i < _size; ++i) {
        Comparator c{0, 0};
        c.read(f);

        push_back(c);
      }
    }

    template <uint8_t N, uint8_t K> std::string Network<N, K>::to_string(sequence_t s) const {
      sequence_t sOutput{};
      if (s > 0) {
        sOutput = run(s);
      }
      std::string output{};

      // by default networks as displayed as a graphically using +, | and -.
      // the compact version simply list the comparators as ordered sets
      // with node numbers in a left to right, top-down traversal.
      // eg: "(1, 2); (3, 4); (1, 4);"
      std::size_t i{0};
      for (const auto &c : comparators) {
        if (i == comparators.size()) {
          break;
        }
        output += c.to_string<N>() + " ";
        i++;
      }

      if (sOutput > 0) {
        output += "\n" + std::to_string(s) + " => " + std::to_string(sOutput);
      }

      return output + "\n";
    }

    template <uint8_t N, uint8_t K> std::string Network<N, K>::knuthDiagram(sequence_t s) const {
      sequence_t sOutput{};
      if (s > 0) {
        sOutput = run(s);
      }
      std::string output{};

      for (auto i = 0; i < N; i++) {
        auto ir = (N - 1) - i;
        // vertex / node
        output += s == 0 ? " " : std::to_string((s >> ir) & 1);
        for (const auto c : comparators) {
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
        for (const auto c : comparators) {
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
  }
  }