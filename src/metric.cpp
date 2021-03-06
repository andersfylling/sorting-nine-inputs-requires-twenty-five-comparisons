#include "sortnet/metric.h"

namespace sortnet {
std::string MetricLayer::to_string() const {
  const auto prunedPercentage = FloatPrecision((Pruned * 1.0 / Generated * 1.0) * 100.0, 2);
  const auto duration = FloatPrecision(DurationGenerating + DurationPruning, 4);

  std::string str{"Layer " + std::to_string(Layer) + "\n"
                  + "\tnetworks: " + std::to_string(Generated - Pruned) + "\n"
                  + "\tgenerated: " + std::to_string(Generated) + "\n"
                  + "\tpruned: " + std::to_string(Pruned) + " (" + std::to_string(prunedPercentage)
                  + "%)\n" + "\tduration: " + std::to_string(duration) + "s\n"};
  return str;
}

void to_json(nlohmann::json &j, const MetricLayer &m) { m.to_json(j); }

void MetricLayer::to_json(::nlohmann::json &j) const {
  auto add = [&](const std::string &name, const auto &v) { j[name] = v; };
  auto addST = [&](const std::string &name, const auto &called, const auto &accepted,
                   const auto &rejected) {
    j[name]["called"] = called;
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

  j["duration"]["pruning"]["total"] = DurationPruning;
  j["duration"]["pruning"]["within_file"] = durationPruningWithinFile;
  j["duration"]["pruning"]["within_cluster"] = durationPruningWithinCluster;
  j["duration"]["pruning"]["across_clusters"] = durationPruningAcrossClusters;

  j["fragmenting"]["before"] = fragmentedBefore;
  j["fragmenting"]["after"] = fragmentedAfter;

  for (const auto &pair : clusterBySize) {
    j["clusters"]["size"][std::to_string(pair.first)] = pair.second;
  }
}

}  // namespace sortnet