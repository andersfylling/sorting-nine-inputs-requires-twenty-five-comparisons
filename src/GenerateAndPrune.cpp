#include "sortnet/GenerateAndPrune.h"

#include <algorithm>
#include <bit>
#include <boost/filesystem.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <vector>

#include "analysis.h"
#include "metadataTree.h"
#include "sortnet/comparator.h"
#include "sortnet/progress.h"
#include "sortnet/sequence.h"
#include "vendors/github.com/haarcuba/cpp-text-table/TextTable.h"

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
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::pruneAcrossClusters(uint8_t layer) {
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
      I3Sorted1(begin, end, begin2, end2);
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
  Progress bar("pruning    :: across clusters     ", nrOfFiles(), 45);
  bar.display();
#endif

  auto* buffer = buffers.get();
  auto& sets = buffer->setsFirst;
  std::vector<std::future<uint32_t>> results{};
  uint64_t pruned{0};
  auto     cluster = filenames.begin();
  for (; cluster != filenames.cend(); ++cluster) {
    for (auto i{0}; i < cluster->files.size(); ++i) {
      const auto &filesA{cluster->files.at(i)};
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      auto fileASize = storage.Load(filesA.set, layer, sets.begin(), sets.end());
      if (fileASize == 0) {
        continue;
      }

      auto secondCluster{cluster};
      for (++secondCluster; secondCluster != filenames.cend(); ++secondCluster) {
#if (PREFER_SAFETY == 1)
        if (secondCluster->size < cluster->size) {
          throw std::logic_error("expected next cluster to have a larger output set size");
        }
#endif
        // file B loop start
        const auto end{secondCluster->files.size()};
        for (auto j{0}; j < end; ++j) {
          const auto &filesB{secondCluster->files.at(j)};
          results.emplace_back(pool.add(prune, filesB.set, sets.cbegin(), sets.cbegin() + fileASize));
        }  // end file B loop
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
  }
#if (PRINT_PROGRESS == 1)
  bar.done();
#endif

  return pruned;
}

template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::pruneWithinClusters(uint8_t layer) {
  auto prune = [&](const auto& filename, auto begin, auto end) -> uint32_t {
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
    const auto originalSize{size};

    auto end2 = begin2 + size;
    I2Sorted1(buffer->positions, begin, end, begin2, end2);
    size = shiftRedundant(begin2, end2);

    const uint32_t pruned = originalSize - size;
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
  Progress bar("pruning    :: within clusters     ", filenames.size(), 45);
  bar.display();
#endif

  auto* buffer = buffers.get();
  auto& sets = buffer->setsFirst;
  uint64_t pruned{0};
  std::vector<std::future<uint32_t>> results{};
  for (const auto &cluster : filenames) {
    for (auto i{0}; i < cluster.files.size(); ++i) {
      const auto &filesA{cluster.files.at(i)};
      const auto& filename{filesA.set};
      auto fileASize = storage.Load(filename, layer, sets.begin(), sets.end());
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      if (fileASize == 0) {
        continue;
      }
      const auto originalFileASize{fileASize};
      auto begin{sets.begin()};
      auto end{sets.begin() + fileASize};

      // file B loop start
      for (auto j{i+1}; j < cluster.files.size(); ++j) {
        const auto &filesB{cluster.files.at(j)};
        results.emplace_back(pool.add(prune, filesB.set, begin, end));
      }  // end file B loop

      pool.wait();

      for (auto& r : results) {
        const auto diff = r.get();
        pruned += diff;
      }
      results.clear();

      // write results to file
      buffers.markPositions(begin, end);
      buffers.clearBuffers();

      fileASize = shiftRedundant(begin, end);
      pruned += originalFileASize - fileASize;
      storage.Save(filename, begin, begin+fileASize);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileWrite++;
#endif
    }
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
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::pruneFilesSeparately(uint8_t layer) {
  auto prune = [&](const auto &file) -> uint64_t {
    auto* buffer = buffers.get();
    auto begin = buffer->setsFirst.begin();
    auto end = buffer->setsFirst.end();

    auto size = storage.Load(file.set, layer, begin, end);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead++;
#endif
    const auto originalSize{size};

    I1Sorted1(begin, begin + size);
    size = shiftRedundant(begin, begin + size);
    if (size == originalSize) {
      return 0;
    }

    // write results to file
    storage.Save(file.set, begin, begin + size);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite++;
#endif

    buffers.put(buffer);
    return originalSize - size;
  };

#if (PRINT_PROGRESS == 1)
  Progress bar("pruning    :: within files        ", filenames.size(), 45);
  bar.display();
#endif

  std::vector<std::future<uint64_t>> results{};
  uint64_t pruned{0};
  for (const auto &cluster : filenames) {
    for (const auto &file : cluster.files) {
      results.emplace_back(pool.add(prune, file));
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

/////////////////////
//
// FRAGMENT
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
    typename Storage>
void GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::fragment(uint8_t layer) {
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
    const auto netFile = storage.Save(layer, itNets, endNets, "f");
    const auto setFile = storage.Save(layer, itSets, endSets, "f");
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
  Progress bar("cleanup    :: fragmenting clusters", existingFiles.size(), 45);
  bar.display();
#endif

  for (const auto &cluster : existingFiles) {
    read(cluster.files, layer, [&](const Net& net, const Set& set){
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
}


/////////////////////
//
// SORT
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
    typename Storage>
void GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::sortFileContent(uint8_t layer) {
  auto sort = [&](const auto &file) -> uint64_t {
    auto* buffer = buffers.get();
    auto& sets = buffer->setsFirst;
    auto begin = sets.begin();

    auto size = storage.Load(file.set, layer, begin, sets.end());
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead++;
#endif

    MetadataTree<N, K> tree(&metadataNodePool, &metadataLeafPool);
    uint64_t clusters{0};
    for (auto i{0}; i < size; ++i) {
      const auto& metadata{sets.at(i).metadata};
      if (tree.push_back(metadata, i)) {
        ++clusters;
      }
    }

    uint32_t counter{0};
    auto handler = [&](const std::vector<uint32_t>& positions) {
      for (const auto pos : positions) {
        // TODO: networks
        buffer->setsSecond.at(counter) = buffer->setsFirst.at(pos);
        ++counter;
      }
    };
    tree.sort(handler);
    if (counter != size) {
      throw std::logic_error("nr of loaded sets differs after sorting");
    }

    // write results to file
    storage.Save(file.set, buffer->setsSecond.begin(), buffer->setsSecond.begin() + counter);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite++;
#endif

    buffers.put(buffer);
    return clusters;
  };

#if (PRINT_PROGRESS == 1)
  Progress bar("cleanup    :: sorting files       ", filenames.size(), 45);
  bar.display();
#endif

//  std::vector<std::future<uint64_t>> results{};
//  uint64_t sum{0};
  for (const auto &cluster : filenames) {
    for (const auto &file : cluster.files) {
//      results.emplace_back(pool.add(sort, file));
      pool.add(sort, file);
    }
    pool.wait();

//    uint64_t clusters{0};
//    for (auto& r : results) {
//      const auto diff = r.get();
//      clusters += diff;
//    }
//    results.clear();
//    sum += clusters / cluster.files.size();

#if (PRINT_PROGRESS == 1)
    ++bar;
    bar.display();
#endif
  }
#if (PRINT_PROGRESS == 1)
  bar.done();
#endif

//  sum /= filenames.size();
//  std::cout << sum << std::endl;
}

/////////////////////
//
// RUN
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, SeqSet Set, ComparatorNetwork Net,
          typename Storage>
MetricsLayered<N, K> GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::run() {
  metric         = &metrics.at(0);
  const auto dir = folder();
  // ensure path exist and is empty
  {
    const auto path = boost::filesystem::path(dir);
    boost::filesystem::remove_all(path);
    boost::filesystem::create_directory(path);
  }

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

  auto generate_clustersBySize = [&](const auto layer) {
#if (RECORD_INTERNAL_METRICS == 1)
    const auto start = now();
#endif
    const auto generated = generateAsClusterBySize(layer);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->Generated = generated;
    metric->DurationGenerating = duration(start, now());
#endif

    return generated;
  };

  auto sort_withinFiles = [&](const auto layer) {
    const auto start     = now();
    sortFileContent(layer);
    return duration(start, now());
  };

  auto fragment_withinClusters = [&](const auto layer) {
    const auto start     = now();
#if (RECORD_INTERNAL_METRICS == 1)
    metric->fragmentedBefore = nrOfFiles();
#endif
    fragment(layer);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->fragmentedAfter = nrOfFiles();
#endif
    return duration(start, now());
  };

  auto prune_withinFiles = [&](const auto layer) {
#if (RECORD_INTERNAL_METRICS == 1)
    const auto start = now();
#endif
    const auto pruned = pruneFilesSeparately(layer);
#if (RECORD_INTERNAL_METRICS == 1)
    const auto end    = now();
    const auto d = duration(start, end);

    metric->prunedWithinFile = pruned;
    metric->Pruned += pruned;

    metric->durationPruningWithinFile = d;
    metric->DurationPruning += d;
#endif

    return pruned;
  };

  auto prune_withinClusters = [&](const auto layer) {
#if (RECORD_INTERNAL_METRICS == 1)
    const auto start = now();
#endif
    const auto pruned = pruneWithinClusters(layer);
#if (RECORD_INTERNAL_METRICS == 1)
    const auto end    = now();
    const auto d = duration(start, end);

    metric->prunedWithinCluster = pruned;
    metric->Pruned += pruned;

    metric->durationPruningWithinCluster = d;
    metric->DurationPruning += d;
#endif

    return pruned;
  };

  auto prune_acrossClusters = [&](const auto layer) {
#if (RECORD_INTERNAL_METRICS == 1)
    const auto start = now();
#endif
    const auto pruned = pruneAcrossClusters(layer);
#if (RECORD_INTERNAL_METRICS == 1)
    const auto end    = now();
    const auto d = duration(start, end);

    metric->prunedAcrossClusters = pruned;
    metric->Pruned += pruned;

    metric->durationPruningAcrossClusters = d;
    metric->DurationPruning += d;
#endif

    return pruned;
  };

  // #############################################
  //
  // iteratively generate and prune networks
  storeEmptyNetworkWithOutputSet();
  bool reversePruningOrder{false};
  uint64_t prevNrOfFilters{0};
  for (uint8_t layer{1}; layer <= K; ++layer) {
    metric = &metrics.at(layer);

    // layer insight
#if (PRINT_LAYER_SUMMARY == 1)
    std::cout << std::endl
              << "======== N" << int(N) << "K" << int(layer)
              << " =========================================="
              << "==========================================="
              << std::endl;
#endif

    // GENERATE
    const auto generated = generate_clustersBySize(layer);

#if (RECORD_IO_TIME == 1)
    const auto fileIODurationGen = nanosecondsToSeconds(storage.duration);
#endif

    // analysis networks/files
//#if (RECORD_ANALYSIS == 1) && (RECORD_INTERNAL_METRICS == 1)
//    metric->clusterBySize = analyzeClusterBySize(layer);
//#endif

    sort_withinFiles(layer);

    uint64_t pruned{0};
    if (!reversePruningOrder) {
      pruned += prune_withinFiles(layer);
      pruned += prune_withinClusters(layer);
    } else {
      pruned += prune_acrossClusters(layer);
    }

    if (nrOfFiles() > 200 || reversePruningOrder) {
      fragment_withinClusters(layer);
      sort_withinFiles(layer);
    }

    if (!reversePruningOrder) {
      pruned += prune_acrossClusters(layer);
    } else {
      pruned += prune_withinFiles(layer);
      pruned += prune_withinClusters(layer);
      fragment_withinClusters(layer); // cleanup for generate
    }

    // detect peak
    const auto nrOfFilters{generated - pruned};
    if (prevNrOfFilters >= nrOfFilters ||
        (layer > 9 && metric->durationPruningWithinCluster > metric->durationPruningAcrossClusters)) {
      reversePruningOrder = true;
    }
    prevNrOfFilters = nrOfFilters;

#if (RECORD_IO_TIME == 1)
    const auto fileIODuration = nanosecondsToSeconds(storage.duration);
    storage.duration = 0;
#endif

    // layer insight
#if (PRINT_LAYER_SUMMARY == 1)
    std::cout << std::endl;
    TextTable t('-', '|', '+');
    t.setAlignment(0, TextTable::Alignment::LEFT);
    t.setAlignment(1, TextTable::Alignment::RIGHT);
    t.setAlignment(2, TextTable::Alignment::RIGHT);
    t.setAlignment(3, TextTable::Alignment::RIGHT);

    auto dec = [](const double_t v){
      const auto precision = 2;
      return std::to_string(v).substr(0, std::to_string(v).find(".") + precision + 1);
    };

    t.add(""); t.add("Time"); t.add("Filters");
#if (RECORD_IO_TIME == 1)
    t.add("IO time");
#endif
    t.endOfRow();

    t.add("Generating");
    t.add(dec(metric->DurationGenerating) + "s");
    t.add(std::to_string(metric->Generated));
#if (RECORD_IO_TIME == 1)
    t.add(dec(fileIODurationGen));
#endif
    t.endOfRow();

    t.add("Pruning");
    t.add(dec(metric->DurationPruning) + "s");
    t.add(std::to_string(metric->Pruned));
#if (RECORD_IO_TIME == 1)
    t.add("?");
#endif
    t.endOfRow();

    t.add("Sum");
    t.add(dec(metric->DurationGenerating + metric->DurationPruning) + "s");
    t.add(std::to_string(metric->Generated - metric->Pruned));
#if (RECORD_IO_TIME == 1)
    t.add(dec(fileIODuration));
#endif
    t.endOfRow();

    t.add("Total so far");
    t.add(dec(metrics.Seconds()) + "s");
    t.add("?");
#if (RECORD_IO_TIME == 1)
    t.add("?");
#endif
    t.endOfRow();

    t.setAlignment( 2, TextTable::Alignment::RIGHT );
    std::cout << t << std::flush;
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



/////////////////////
//
// ANALYSIS
//
////////////////////
template <uint8_t N, uint8_t K, uint8_t NrOfCores, typename Set, typename Net, typename Storage>
std::map<uint64_t, uint64_t>
GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::analyzeClusterBySize(uint8_t layer) {
  auto map = std::map<uint64_t, uint64_t>();

  for (const auto &cluster : filenames) {
    read(cluster.files, layer, [&](const Net& net, const Set& set){
      map = ::analysis::cluster::sizeCount<Set>(map, set);
    });
  }

  return map;
}


template <uint8_t N, uint8_t K, uint8_t NrOfCores, typename Set, typename Net, typename Storage>
uint64_t GenerateAndPrune<N, K, NrOfCores, Set, Net, Storage>::nrOfFiles() const {
  std::size_t size{0};
  for (const auto& cluster : filenames) {
    size += cluster.files.size();
  }
  return size;
}