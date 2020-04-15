#include "sortnet/metric.h"

std::string MetricLayer::to_string() const {
  const auto prunedPercentage = FloatPrecision((Pruned * 1.0 / Generated * 1.0) * 100.0, 2);
  const auto duration         = FloatPrecision(DurationGenerating + DurationPruning, 4);

  std::string str{"Layer " + std::to_string(Layer) + "\n" +
                  "\tnetworks: " + std::to_string(Generated - Pruned) + "\n" +
                  "\tgenerated: " + std::to_string(Generated) + "\n" +
                  "\tpruned: " + std::to_string(Pruned) + " (" + std::to_string(prunedPercentage) +
                  "%)\n" + "\tduration: " + std::to_string(duration) + "s\n"};
  return str;
}

void MetricLayer::to_json(::nlohmann::json &j) const {
  auto add = [&](const std::string &name, const auto &v) {
    j[name] = v;
  };
  auto addST = [&](const std::string &name, const auto &called, const auto &accepted,
                   const auto &rejected) {
    j[name]["called"]   = called;
    j[name]["accepted"] = accepted;
    j[name]["rejected"] = rejected;
  };

  add("layer", Layer);
  add("file_reads", FileRead);
  add("file_writes", FileWrite);
  add("redundant_comparators", RedundantComparator);
  add("redundant_comparators_quick", RedundantComparatorQuick);
  addST("st1", ST1Calls, ST1, ST1Calls - ST1);
  addST("st2", ST2Calls, ST2, ST2Calls - ST2);
  addST("st3", ST3Calls, ST3, ST3Calls - ST3);
  add("subsumptions", Subsumptions);
  add("permutations", Permutations);
  add("subsumes_fallback", SubsumesCalls);

  j["generated"]["total"] = Pruned;

  j["pruned"]["total"] = Pruned;
  j["pruned"]["within_file"] = prunedWithinFile;
  j["pruned"]["within_cluster"] = prunedWithinCluster;
  j["pruned"]["across_clusters"] = prunedAcrossClusters;

  j["duration"]["generating"]["total"] = DurationGenerating;

  j["duration"]["pruning"]["total"]    = DurationPruning;
  j["duration"]["pruning"]["within_file"]    = durationPruningWithinFile;
  j["duration"]["pruning"]["within_cluster"]    = durationPruningWithinCluster;
  j["duration"]["pruning"]["across_clusters"]    = durationPruningAcrossClusters;

  j["fragmenting"]["before"] = fragmentedBefore;
  j["fragmenting"]["after"] = fragmentedAfter;

  for (const auto& pair : clusterBySize) {
    j["clusters"]["size"][std::to_string(pair.first)] = pair.second;
  }
}

template <uint8_t N, uint8_t K>
std::string MetricsLayered<N, K>::String() const {
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

template <uint8_t N, uint8_t K>
std::string MetricsLayered<N, K>::HeaderString(uint8_t maxSize) {
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

std::string comparisonTable(const MetricLayer &n7k9, const MetricLayer &n7k10) {
  auto                          i{0};
  const std::array<uint8_t, 10> width{
      7, 10, 13, 7, 11, 7, 10, 7, 10, 10,
  };
  auto getWidth = [&]() {
    i = i % width.size();
    return width[i++];
  };

  std::stringstream ss{};
  auto              addCol = [&](auto content) {
    ss << "|" << std::setw(getWidth()) << content << " ";
  };
  auto endRow = [&]() {
    i = 0;
    ss << "|" << std::endl;
  };

  auto createHeader = [&]() {
    addCol("N\\k");
    addCol(" |N(kn)| ");
    addCol("total");
    addCol("sub");
    addCol("perm1");
    addCol("time1");
    addCol("perm2");
    addCol("time2");
    addCol("perm3");
    addCol("time3");
    endRow();

    ss << std::string(ss.str().size(), '-') << std::endl;
  };
  createHeader();

  addCol("(7,9)");
  addCol(678);
  addCol("1,223,426");
  addCol("5,144");
  addCol("26,505,101");
  addCol(2.88);
  addCol("33,120");
  addCol(0.07);
  addCol(n7k9.Permutations);
  addCol(n7k9.DurationPruning);
  endRow();

  addCol("(7,10)");
  addCol(510);
  addCol("878,995");
  addCol("5,728");
  addCol("25,363,033");
  addCol(2.82);
  addCol("24,362");
  addCol(0.06);
  addCol(n7k10.Permutations);
  addCol(n7k10.DurationPruning);
  endRow();

  auto table{ss.str()};
  return table;
}



template <uint8_t N, uint8_t K>
::nlohmann::json MetricsLayered<N, K>::to_json(const uint8_t cores, const std::size_t fileLimit) const {
  ::nlohmann::json j;
  j["n"]             = N;
  j["k"]             = K;
  j["duration"]      = Seconds();
  j["duration_type"] = "seconds";
  j["cores"]         = cores;
  j["layers"]        = metrics;
  j["file_limit"]    = fileLimit;

  std::time_t result = std::time(nullptr);
  j["date"]          = std::asctime(std::localtime(&result));
  j["epoch"]         = result;

  return j;
}