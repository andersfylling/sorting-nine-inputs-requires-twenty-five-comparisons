#include "sortnet/persistentStorage.h"

#include <fstream>
#include <iomanip>

#include "sortnet/util.h"

template <ComparatorNetwork Net, SeqSet Set, uint8_t N, uint8_t K>
template <typename _II, typename _II2>
std::string PersistentStorage<Net, Set, N, K>::Save(const std::string &filename, _II begin,
                                                    const _II2 end) {
#if (RECORD_IO_TIME == 1)
  const auto start = std::chrono::steady_clock::now();
#endif
  // TODO: add filter argument, using a boolean lambda
  std::ofstream f{filename, std::ios::binary | std::ios::trunc};
  f.unsetf(std::ios_base::skipws);

  // number of networks/sets
  const uint32_t distance{static_cast<uint32_t>(std::distance(begin, end))};
  binary_write(f, distance);

  // the actual content
  for (; begin != end; ++begin) {
    begin->write(f);
  }
  f.close();
#if (RECORD_IO_TIME == 1)
  const auto stop = std::chrono::steady_clock::now();
  duration += std::chrono::duration_cast<std::chrono::nanoseconds> (stop - start).count();
#endif

  return std::string(filename);
}

template <ComparatorNetwork Net, SeqSet Set, uint8_t N, uint8_t K>
void PersistentStorage<Net, Set, N, K>::Save(const std::string &     filename,
                                             const ::nlohmann::json &content) const {
  std::ofstream f{dir + filename, std::ios::out | std::ios::trunc};
  f << std::setw(2) << content << std::endl;
  f.close();
}

template <ComparatorNetwork Net, SeqSet Set, uint8_t N, uint8_t K>
template <typename iterator>
uint32_t PersistentStorage<Net, Set, N, K>::Load(const std::string &filename, const uint8_t layer,
                                                 iterator it, const iterator end) {
#if (RECORD_IO_TIME == 1)
  const auto start = std::chrono::steady_clock::now();
#endif
  std::ifstream f{filename, std::ios::in | std::ios::binary};
  f.unsetf(std::ios_base::skipws);

  uint32_t limit{};
  binary_read(f, limit);

  uint32_t counter{0};
  for (; it != end && counter < limit; ++it) {
    it->read(f, layer);
    ++counter;
  }

  f.close();
#if (RECORD_IO_TIME == 1)
  const auto stop = std::chrono::steady_clock::now();
  duration += std::chrono::duration_cast<std::chrono::nanoseconds> (stop - start).count();
#endif

  return counter;
}
