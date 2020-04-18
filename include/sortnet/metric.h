#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

#include "json.h"
#include "util.h"

namespace sortnet {
  class MetricLayer {
  public:
    uint8_t Layer{0};

    uint64_t FileRead{0};
    uint64_t FileWrite{0};

    uint64_t Generated{0};
    uint64_t Pruned{0};
    uint64_t prunedWithinFile{0};
    uint64_t prunedWithinCluster{0};
    uint64_t prunedAcrossClusters{0};

    uint64_t fragmentedBefore{0};
    uint64_t fragmentedAfter{0};

    uint64_t RedundantComparator{0};
    uint64_t RedundantComparatorQuick{0};

    uint64_t HasNoPermutation{0};
    uint64_t PermutationGeneratorCalls{0};

    uint64_t ST1Calls{0};
    uint64_t ST1{0};
    uint64_t ST2Calls{0};
    uint64_t ST2{0};
    uint64_t ST3Calls{0};
    uint64_t ST3{0};
    uint64_t ST4Calls{0};
    uint64_t ST4{0};
    uint64_t ST5Calls{0};
    uint64_t ST5{0};
    uint64_t ST6Calls{0};
    uint64_t ST6{0};

    std::map<uint64_t, uint64_t> clusterBySize{};

    uint64_t sizeClusters{0};

    uint64_t SubsumesCalls{0};
    uint64_t Subsumptions{0};

    uint64_t Permutations{0};

    double DurationGenerating{0};
    double DurationPruning{0};

    double durationPruningWithinFile{0};
    double durationPruningWithinCluster{0};
    double durationPruningAcrossClusters{0};

    [[nodiscard]] constexpr uint64_t filters() const {
      return Generated - Pruned;
    }

    [[nodiscard]] std::string to_string() const;
    void to_json(::nlohmann::json &j) const;
  };

  std::string comparisonTable(const MetricLayer &n7k9, const MetricLayer &n7k10);

  template <uint8_t N, uint8_t K> class MetricsLayered {
  private:
    static const uint8_t StringViewMinSize{1};

  public:
    std::array<MetricLayer, K + 1> metrics{};

    constexpr MetricsLayered() {
      uint8_t depth{0};
      for (auto &m : metrics) {
        m.Layer = depth;
        ++depth;
      }
    };

    template <typename T> requires std::is_integral<T>::value MetricLayer &operator[](T i) {
      return metrics[i];
    }

    template <typename T> requires std::is_integral<T>::value MetricLayer &at(T i) {
      return metrics.at(i);
    }

    [[nodiscard]] double Seconds() const {
      double duration{0};
      for (const auto &layer : metrics) {
        duration += layer.DurationGenerating + layer.DurationPruning;
      }
      return duration;
    }

    std::string HeaderString(uint8_t maxSize) {
      std::stringstream ss{};

      ss << std::setw(10) << "N\\k";
      ss << " |";
      for (auto i = StringViewMinSize; i <= maxSize; i++) {
        ss << std::setw(10) << int(i);
        if (i % 10 == 0) {
          ss << "'";
        }
      }
      ss << std::endl;

      auto str{ss.str()};
      return str + std::string(str.size(), '-');
    }

    [[nodiscard]] std::string String() const {
      std::stringstream ss{};

      ss << std::setw(10) << int(N);
      ss << " |";
      for (uint8_t i = StringViewMinSize; i < metrics.size(); i++) {
        const auto &m{metrics[i]};
        ss << std::setw(10) << (m.Generated - m.Pruned);
        if (i % 10 == 0) {
          ss << "'";
        }
      }

      return ss.str();
    }

    [[nodiscard]] ::nlohmann::json to_json(uint8_t cores, std::size_t fileLimit) const {
      ::nlohmann::json j;
      j["n"] = N;
      j["k"] = K;
      j["duration"] = Seconds();
      j["duration_type"] = "seconds";
      j["cores"] = cores;
      j["layers"] = metrics;
      j["file_limit"] = fileLimit;

      std::time_t result = std::time(nullptr);
      j["date"] = std::asctime(std::localtime(&result));
      j["epoch"] = result;

      return j;
    }
  };

  [[maybe_unused]] static void to_json(::nlohmann::json &j, const MetricLayer &m) { m.to_json(j); }
}