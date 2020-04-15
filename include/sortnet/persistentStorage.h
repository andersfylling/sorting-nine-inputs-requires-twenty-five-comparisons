#pragma once

#include <array>
#include <chrono>
#include <map>
#include <string>

#include "../config/config.h"
#include "json.h"
#include "sortnet/sets/concept.h"
#include "util.h"

template <ComparatorNetwork Net, SeqSet Set, uint8_t N, uint8_t K>
class PersistentStorage {
 private:
  const uint8_t width{7};

  const std::string                                  PrefixNetworks{"n"};
  const std::string                                  PrefixSets{"o"};
  const std::string                                  FilenameExt{"gnp"};
  const std::string                                  dir{createFolder()};
  std::map<std::string, std::array<uint64_t, K + 1>> snrs{};

  [[nodiscard]] std::string fileName(const std::string &prefix, uint8_t size, uint64_t snr) const {
    const auto NStr = std::to_string(N);

    auto sizeStr = std::to_string(size);
    sizeStr      = std::string(3 - sizeStr.size(), '0').append(sizeStr);

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

  inline std::string createFilename(const Net &network, uint8_t layer) {
    return this->createFilename(PrefixNetworks, layer);
  }

  inline std::string createFilename(const Set &set, uint8_t layer) {
    return this->createFilename(PrefixSets, layer);
  }

  [[nodiscard]] std::string createFolder() const {
    const auto NStr = std::to_string(N);
    return "./network" + NStr + "/";
  }

 public:
#if (RECORD_IO_TIME == 1)
  uint64_t duration{};
#endif

  std::string filenameNetworks(uint8_t size, uint64_t snr) {
    return this->fileName(PrefixNetworks, size, snr);
  }
  std::string filenameSets(uint8_t size, uint64_t snr) {
    return this->fileName(PrefixSets, size, snr);
  }
  std::string folder() { return dir; }

  template <typename _II, typename _II2>
  std::string Save(const std::string &filename, _II begin, _II2 end);
  template <typename _II, typename _II2>
  std::string Save(uint8_t layer, _II begin, const _II2 end, const std::string& prefix = "") {
    std::string filename{dir + prefix + createFilename(*begin, layer)};
    return Save(filename, begin, end);
  }

  void Save(const std::string &filename, const ::nlohmann::json &content) const;

  template <typename iterator>
  uint32_t Load(const std::string &filename, uint8_t layer, iterator begin, iterator end);
};
