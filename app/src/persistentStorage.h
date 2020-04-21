#pragma once

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>

#include "sortnet/concepts.h"
#include "sortnet/io.h"
#include "sortnet/json.h"
#include "sortnet/z_environment.h"

template <::sortnet::concepts::ComparatorNetwork Net, ::sortnet::concepts::Set Set, uint8_t N,
          uint8_t K>
class PersistentStorage {
private:
  const uint8_t width{7};

  const std::string PrefixNetworks{"n"};
  const std::string PrefixSets{"o"};
  const std::string FilenameExt{"gnp"};
  const std::string dir{createFolder()};
  std::map<std::string, std::array<uint64_t, K + 1>> snrs{};

  [[nodiscard]] std::string fileName(const std::string &prefix, uint8_t size, uint64_t snr) const {
    const auto NStr = std::to_string(N);

    auto sizeStr = std::to_string(size);
    sizeStr = std::string(3 - sizeStr.size(), '0').append(sizeStr);

    auto snrStr = std::to_string(snr);
    if (snrStr.size() < width) {
      snrStr = std::string(width - snrStr.size(), '0').append(snrStr);
    }

    return prefix + NStr + "-" + sizeStr + "-" + snrStr + "." + FilenameExt;
  }

  // filename format:
  // {prefix}-{N}-{network size}-{snr}.cnets
  std::string createFilename(const std::string &prefix, const uint8_t size) {
    const auto nr = this->snrs[prefix][size];
    this->snrs[prefix][size]++;

    return this->fileName(prefix, size, nr);
  }

  inline std::string createFilename([[maybe_unused]] const Net &network, uint8_t layer) {
    return this->createFilename(PrefixNetworks, layer);
  }

  inline std::string createFilename([[maybe_unused]] const Set &set, uint8_t layer) {
    return this->createFilename(PrefixSets, layer);
  }

  [[nodiscard]] std::string createFolder() const {
    const auto NStr = std::to_string(N);
    return "network" + NStr + "/";
  }

public:
#if (RECORD_IO_TIME == 1)
  uint64_t duration{};
#endif

  explicit PersistentStorage(const bool clearDir) {
    if (clearDir) {
      const auto pwd = std::filesystem::current_path();
      std::filesystem::remove_all(pwd / dir);
      std::filesystem::create_directories(pwd / dir);
    }
  }

  PersistentStorage() { PersistentStorage(true); }

  std::string filenameNetworks(uint8_t size, uint64_t snr) {
    return this->fileName(PrefixNetworks, size, snr);
  }
  std::string filenameSets(uint8_t size, uint64_t snr) {
    return this->fileName(PrefixSets, size, snr);
  }
  std::string folder() { return dir; }

  template <typename II, typename II2>
  std::string Save(const std::string &filename, II begin, II2 end) {
#if (RECORD_IO_TIME == 1)
    const auto start = std::chrono::steady_clock::now();
#endif
    std::ofstream f{filename, std::ios::binary | std::ios::trunc};
    f.unsetf(std::ios_base::skipws);

    // number of networks/sets
    const int32_t distance{static_cast<int32_t>(std::distance(begin, end))};
    binary_write(f, distance);

    // the actual content
    for (; begin != end; ++begin) {
      begin->write(f);
    }
    f.close();
#if (RECORD_IO_TIME == 1)
    const auto stop = std::chrono::steady_clock::now();
    duration += std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
#endif

    return std::string(filename);
  }

  template <typename II, typename II2>
  std::string Save(uint8_t layer, II begin, const II2 end, const std::string &prefix = "") {
    std::string filename{dir + prefix + createFilename(*begin, layer)};
    return Save(filename, begin, end);
  }

  void Save(const std::string &filename, const ::nlohmann::json &content) const {
    std::ofstream f{dir + filename, std::ios::out | std::ios::trunc};
    f << std::setw(2) << content << std::endl;
    f.close();
  }

  template <typename iterator> uint32_t Load(const std::string &filename,
                                             [[maybe_unused]] uint8_t layer, iterator it,
                                             iterator end) {
#if (RECORD_IO_TIME == 1)
    const auto start = std::chrono::steady_clock::now();
#endif
    std::ifstream f{filename, std::ios::in | std::ios::binary};
    f.unsetf(std::ios_base::skipws);

    int32_t limit{};
    binary_read(f, limit);

    int32_t counter{0};
    for (; (it != end && counter < limit); ++it) {  //&& (layer <= 2 && counter < 3)
      it->read(f);
      ++counter;
    }

    f.close();
#if (RECORD_IO_TIME == 1)
    const auto stop = std::chrono::steady_clock::now();
    duration += std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
#endif

    return counter;
  }
};