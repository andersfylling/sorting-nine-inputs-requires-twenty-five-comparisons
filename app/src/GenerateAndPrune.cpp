#include "GenerateAndPrune.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <tabulate/table.hpp>
#include <vector>

#include "progress.h"
#include "sortnet/comparator.h"
#include "sortnet/sequence.h"

/////////////////////
//
// GENERATE
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::generate(const uint8_t layer) {
  constexpr uint32_t FileSize{NetworksPerFileLimit};

  std::vector<Set> setBuf(FileSize);
  std::vector<Net> netBuf(FileSize);

  uint64_t counter = 0;

  auto find = [](const uint64_t netID, auto it, const auto end) -> Net {
    for (; it != end; ++it) {
      if (it->id == netID) {
        return static_cast<Net>(*it);
      }
    }

    throw std::logic_error("no network was not found for the given network id");
  };

  Set set{};
  Net network{};

  const auto prevLayer{layer - 1};
  for (const NetAndSetFilename &file : filenames) {
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead += 2;
#endif
    const auto nrOfNets = storage.Load(file.net, prevLayer, netBuf.begin(), netBuf.end());
    const auto nrOfSets = storage.Load(file.set, prevLayer, setBuf.begin(), setBuf.end());

    for (uint64_t i{0}; i < nrOfSets; ++i) {
      const auto netID{setBuf.at(i).metadata.netID};
      network = find(netID, netBuf.begin(), netBuf.begin() + nrOfNets);

      for (const auto c : ::comparator::all<N>) {
        if (network.last() == c) {
#if (RECORD_INTERNAL_METRICS == 1)
          metric->RedundantComparatorQuick++;
#endif
          continue;
        }

        // apply new comparator and see if it causes a new change
        set.reset();
        for (auto s : setBuf.at(i)) {
          const auto k{std::popcount(s) - 1};
          s = ::comparator::apply(c, s);
          set.insert(k, s);
        }
        if (set == setBuf.at(i)) {
#if (RECORD_INTERNAL_METRICS == 1)
          metric->RedundantComparator++;
#endif
          continue;
        }

        network.add(c);
        set.computeMeta();
        _f(network, set);
        network.undo();
        ++counter;
      }
    }
  }

  return counter;
}

template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::generateAsClusterBySize(uint8_t layer) {
  std::map<std::size_t, NetAndSet<NetworksPerFileLimit, Set, Net>> buffers{};

  const auto existingFiles = std::move(filenames);
  filenames.clear();  // TODO: redundant?

  uint64_t id = 0;
  auto setNetID = [&](auto itNets, const auto endNets, auto itSets) {
    // link networks and sets by network id
    for (; itNets != endNets; ++itNets, ++itSets) {
      itNets->id    = id;
      itSets->metadata.netID = id;
      ++id;
    }
  };

  auto save = [&](const std::size_t size, auto itNets, const auto endNets, auto itSets,
                  const auto endSets) -> void {
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite += 2;
#endif

    // save to files
    const auto netFile = storage.Save(layer, itNets, endNets);
    const auto setFile = storage.Save(layer, itSets, endSets);
    const auto pair    = NetAndSetFilename{
        .net{netFile},
        .set{setFile},
    };

    // update cluster
    for (auto &cluster : filenames) {
      if (cluster.size != size) {
        continue;
      }

      cluster.files.push_back(pair);
    }
  };

#if (PRINT_PROGRESS == 1)
  Progress bar("generating :: files & clusters    ", existingFiles.size(), 45);
  bar.display();
#endif

  for (const auto &cluster : existingFiles) {
    generate(cluster.files, layer, [&](const Net& net, const Set& set){
      const auto key{set.size()};
      if (!buffers.contains(key)) {
        // add new buffer
        buffers.insert({key, {}});

        // add new cluster
        const auto lower = std::upper_bound(filenames.cbegin(), filenames.cend(), key,
                                            [](const auto size, const auto &cluster) -> bool {
                                              return cluster.size >= size;
                                            });
        filenames.insert(lower, {.size{key}});
      }

      auto &buffer = buffers.at(key);

      if (buffer.add(net, set) == buffer.cend()) {
        setNetID(
            buffer.nets.begin(), buffer.nets.cend(),
            buffer.sets.begin());
        save(key,
             buffer.nets.cbegin(), buffer.nets.cend(),
             buffer.sets.cbegin(), buffer.sets.cend());
        buffer.clear();
      }
    });
#if (PRINT_PROGRESS == 1)
    ++bar;
    bar.display();
#endif
  }

  // check if there is anything else to write to file
  for (auto &pair : buffers) {
    auto &buffer = pair.second;
    if (buffer.size == 0) {
      continue;
    }

    const auto key = pair.first;
    setNetID(
        buffer.nets.begin(), buffer.nets.cbegin() + buffer.size,
        buffer.sets.begin());
    save(key,
         buffer.nets.cbegin(), buffer.nets.cbegin() + buffer.size,
         buffer.sets.cbegin(), buffer.sets.cbegin() + buffer.size);
    buffer.clear();
  }
#if (PRINT_PROGRESS == 1)
  bar.done();
#endif

#if (RECORD_INTERNAL_METRICS == 1)
  metric->sizeClusters = buffers.size();
#endif
  return id;
}

/////////////////////
//
// PRUNE
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::pruneAcrossFiles(uint8_t layer) {
  auto prune = [&](const std::string& filename, auto begin, auto end) -> uint32_t {
    auto* buffer = buffers.get();
    auto begin2 = buffer->setsFirst.begin();

      auto size = storage.Load(filename, layer, begin2, buffer->setsFirst.end());
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      if (size == 0) {
        buffers.put(buffer);
        return 0;
      }
      const auto sizeBeforePruning{size};

      auto end2 = begin2 + size;
      markRedundantNetworks(begin, end, begin2, end2);
      size = shiftRedundant(begin2, end2);

      // write results to file if anything changed
      const uint32_t pruned = sizeBeforePruning - size;
      if (pruned > 0) {
        storage.Save(filename, begin2, begin2 + size);
#if (RECORD_INTERNAL_METRICS == 1)
        metric->FileWrite++;
#endif
      }
      buffers.put(buffer);
      return pruned;
  };


#if (PRINT_PROGRESS == 1)
  // TODO: n'th triangle number
  Progress bar("pruning", "across files", filenames.size());
  bar.display();
#endif

  auto* buffer = buffers.get();
  auto& sets = buffer->setsFirst;
  std::vector<std::future<uint32_t>> results{};
  uint64_t pruned{0};
  for (std::size_t i{0}; i < filenames.size(); ++i) {
    const auto &file{filenames.at(i)};
    auto size = storage.Load(file.set, layer, sets.begin(), sets.end());
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead++;
#endif
    if (size == 0) {
      continue;
    }

    for (std::size_t j{i + 1}; j < filenames.size(); ++j) {
      results.emplace_back(pool.add(prune, file.set, sets.cbegin(), sets.cbegin() + size));
    }
    pool.wait();

    for (auto& r : results) {
      const auto diff = r.get();
      pruned += diff;
    }
    results.clear();
#if (PRINT_PROGRESS == 1)
    ++bar;
      bar.display();
#endif
  }
#if (PRINT_PROGRESS == 1)
  bar.done();
#endif

  return pruned;
}

template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::pruneWithinFiles(uint8_t layer) {
#if (PRINT_PROGRESS == 1)
  std::mutex m;
#endif

  auto updateProgress = [&]() -> void {
#if (PRINT_PROGRESS == 1)
    const std::lock_guard<std::mutex> lock(m);
    ++bar;
    bar.display();
#endif
  };

  auto prune = [&](const auto &file) -> uint64_t {
    auto* buffer = buffers.get();
    auto begin = buffer->setsFirst.begin();
    auto end = buffer->setsFirst.end();

    auto size = storage.Load(file.set, layer, begin, end);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead++;
#endif
    const auto originalSize{size};

    markRedundantNetworks(begin, begin + size);
    size = shiftRedundant(begin, begin + size);
    if (size == originalSize) {
      return 0;
    }

    // write results to file
    storage.Save(file.set, begin, begin + size);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite++;
#endif

    updateProgress();

    buffers.put(buffer);
    return originalSize - size;
  };

#if (PRINT_PROGRESS == 1)
  Progress bar("pruning", "within files", filenames.size());
  bar.display();
#endif

  std::vector<std::future<uint64_t>> results{};
  for (const auto &file : filenames) {
    results.emplace_back(pool.add(prune, file));
  }

  pool.wait();
#if (PRINT_PROGRESS == 1)
  bar.done();
#endif

  uint64_t pruned{0};
  for (auto& r : results) {
    const auto diff = r.get();
    pruned += diff;
  }

  return pruned;
}


/////////////////////
//
// RUN
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
MetricsLayered<N, K> GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::run() {
  metric = &metrics.at(0);

  auto microsecondsToSeconds = [](const double_t mcs){
    return mcs / 1000000.0;
  };
  auto nanosecondsToSeconds = [](const double_t ns){
    return ns / 1000000000.0;
  };
  auto duration = [&](auto start, auto end) {
    const auto d = end - start;
    const auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(d).count();
    return microsecondsToSeconds(mcs);
  };
  auto now = []() -> std::chrono::steady_clock::time_point {
    return std::chrono::steady_clock::now();
  };

  // #############################################
  //
  // iteratively generate and prune networks
  storeEmptyNetworkWithOutputSet();
  for (uint8_t layer{1}; layer <= K; ++layer) {
    metric = &metrics.at(layer);

    // layer insight / progress
#if (PRINT_LAYER_SUMMARY == 1) or (PRINT_PROGRESS == 1)
    std::cout << std::endl
              << "======== N" << int(N) << "K" << int(layer)
              << " =========================================="
              << "==========================================="
              << std::endl;
#endif


    uint64_t generated{0};
    { // GENERATE NETWORKS AND OUTPUT SETS
#if (RECORD_INTERNAL_METRICS == 1)
      const auto start = now();
#endif
      generated += generate(layer);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->Generated = generated;
      metric->DurationGenerating = duration(start, now());
#endif
    }

#if (RECORD_IO_TIME == 1)
    const auto fileIODurationGen = nanosecondsToSeconds(storage.duration);
#endif

    uint64_t pruned{0};
    { // PRUNE WITHIN FILES
#if (RECORD_INTERNAL_METRICS == 1)
      const auto start = now();
#endif
      pruned += pruneWithinFiles(layer);
#if (RECORD_INTERNAL_METRICS == 1)
      const auto end    = now();
      const auto d = duration(start, end);

      metric->prunedWithinFile = pruned;
      metric->Pruned += pruned;

      metric->durationPruningWithinFile = d;
      metric->DurationPruning += d;
#endif
    }

    { // PRUNE ACROSS FILES
#if (RECORD_INTERNAL_METRICS == 1)
      const auto start = now();
#endif
      pruned += pruneAcrossFiles(layer);
#if (RECORD_INTERNAL_METRICS == 1)
      const auto end    = now();
      const auto d = duration(start, end);

      metric->prunedAcrossClusters = pruned;
      metric->Pruned += pruned;

      metric->durationPruningAcrossClusters = d;
      metric->DurationPruning += d;
#endif
    }

#if (RECORD_IO_TIME == 1)
    const auto fileIODuration = nanosecondsToSeconds(storage.duration);
    storage.duration = 0;

    printLayerSummary(layer, fileIODurationGen, fileIODuration);
#endif

    // in case the program needs to terminate before we had planned,
    // at least we have the metrics
#if (SAVE_METRICS == 1)
    const auto json = metrics.to_json(NrOfCores, NetworksPerFileLimit);
    storage.Save("metrics.json", json);
#endif
  }

  return metrics;
}

template <uint8_t N, uint8_t K, uint8_t NrOfCores, typename Set, typename Net, typename Storage>
void
GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::printLayerSummary(uint8_t layer, double_t fileIODurationGen, double_t fileIODuration) {
  auto dec = [](const double_t v) -> std::string {
    const auto precision = 2;
    return std::to_string(v).substr(0, std::to_string(v).find(".") + precision + 1);
  };

  Table t;
  t.add_row({"", "Time", "Filters", "IO Time"});
  std::size_t columns{4};

  std::string ioTime{"-"};
#if (RECORD_IO_TIME == 1)
  ioTime = dec(fileIODurationGen);
#endif
  t.add_row({"Generating", dec(metric->DurationGenerating) + "s", std::to_string(metric->Generated), ioTime});

#if (RECORD_IO_TIME == 1)
  ioTime = "-";
#endif
  t.add_row({"Pruning", dec(metric->DurationPruning) + "s", std::to_string(metric->Pruned), ioTime});

#if (RECORD_IO_TIME == 1)
  ioTime = dec(fileIODuration);
#endif
  t.add_row({"Sum", dec(metric->DurationGenerating + metric->DurationPruning) + "s", std::to_string(metric->Generated - metric->Pruned), ioTime});

#if (RECORD_IO_TIME == 1)
  ioTime = "-";
#endif
  t.add_row({"Total so far", dec(metrics.Seconds()) + "s","-", ioTime});

  for (std::size_t i{1}; i < columns; ++i) {
    t.column(i).format().font_align(FontAlign::right);
  }

  // center-align and color header cells
  for (std::size_t i{0}; i < columns; ++i) {
    t[0][i].format()
        .font_color(Color::yellow)
        .font_align(FontAlign::center)
        .font_style({FontStyle::bold});
  }

  std::cout << std::endl << t << std::endl;
}