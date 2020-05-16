#pragma once

#include <sortnet/concepts.h>
#include <sortnet/json.h>
#include <sortnet/metric.h>
#include <sortnet/permutation.h>
#include <sortnet/z_environment.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <tabulate/table.hpp>
#include <vector>

#include "BufferPool.h"
#include "progress.h"
#include "sortnet/comparator.h"
#include "sortnet/sequence.h"
#include "vendors/github.com/dabbertorres/ThreadPool/ThreadPool.h"

struct NetAndSetFilename {
  const std::string net;
  const std::string set;
};

template <uint8_t N, uint8_t K, uint8_t NrOfCores, ::sortnet::concepts::Set Set,
          ::sortnet::concepts::ComparatorNetwork Net, typename Storage>
class GenerateAndPrune {
private:
  static const auto FileLimit{::sortnet::segment_capacity * 2};  // ugly
  std::vector<NetAndSetFilename> filenames{};
  Storage storage{};

  ::sortnet::BufferPool<Set, Net, NrOfCores, ::sortnet::segment_capacity> buffers{};

  ::sortnet::MetricsLayered<N, K> metrics{};
  ::sortnet::MetricLayer* metric = &metrics.at(0);

  ::dbr::cc::ThreadPool pool;

  constexpr void storeEmptyNetworkWithOutputSet() {
    // we store an empty network and a complete output set
    std::array<Net, 1> _networks{};
    auto& net{_networks.at(0)};
    std::array<Set, 1> _sets{};
    auto& set{_sets.at(0)};

    // cleanup
    net.clear();
    set.clear();

    // create relationship
    net.id = 1;
    set.metadata.netID = net.id;

    // populate the output set with all the sequences from <0, N^2]
    for (const auto s : ::sortnet::sequence::binary::all<N>) {
      const auto k{std::popcount(s) - 1};
      set.insert(k, s);
    }
    set.computeMeta();  // compute metadata, if needed...
    metric->Generated = 1;

    // store
    const auto layer{0};
    const auto netFile = storage.Save(layer, _networks.cbegin(), _networks.cbegin() + 1);
    const auto setFile = storage.Save(layer, _sets.cbegin(), _sets.cbegin() + 1);
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite++;
#endif

    filenames.emplace_back(NetAndSetFilename{
        .net{netFile},
        .set{setFile},
    });
  }

  // group together the unmarked sets
  template <typename _II, typename _II2> constexpr uint64_t shiftRedundant(_II it, const _II2 end) {
    // prune away marked sets by shifting those of interest
    // to the left and decreasing the size.
    auto counter{0};
    bool skewed{false};
    _II it2 = it;
    for (; it != end; ++it) {
      if (it->metadata.marked) {
        skewed = true;
        continue;
      }
      if (skewed) {
        *it2 = *it;
        it->metadata.marked = true;
      }
      ++it2;
      ++counter;
    }

    return counter;
  }

  constexpr bool subsumesByPermutation(const Set& setA, const Set& setB) const {
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST4Calls++;
#endif
    ::sortnet::permutation::constraints_t<N> constraints{};
    ::sortnet::permutation::clear<N>(constraints);
    ::sortnet::permutation::constraints<N>(constraints, setA, setB);

    if (!::sortnet::permutation::valid_fast<N>(constraints)) {
      return false;
    }
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST4++;
    metric->PermutationGeneratorCalls++;
#endif

    return ::sortnet::permutation::generate<N>(
        constraints, [&](const ::sortnet::permutation::permutation_t<N>& p) {
#if (RECORD_INTERNAL_METRICS == 1)
          metric->Permutations++;
#endif
          return ::sortnet::permutation::subsumes<N>(p, setA, setB);
        });
  }

  constexpr bool permutationConditions(const Set& setA, const Set& setB) const {
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST1Calls++;
#endif
    if (!::sortnet::permutation::ST1(setA, setB)) {
      return false;
    }
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST1++;
    metric->ST2Calls++;
#endif
    if (!::sortnet::permutation::ST2(setA, setB)) {
      return false;
    }
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST2++;
    metric->ST3Calls++;
#endif
    if (!::sortnet::permutation::ST3(setA, setB)) {
      return false;
    }
#if (RECORD_INTERNAL_METRICS == 1)
    metric->ST3++;
#endif

    return true;
  }

  // compare two sets and check if they can be subsumed by a permutation
  // return true if the first set is marked (allowing fail fast)
  constexpr bool marked(Set& setA, Set& setB) const {
    if (permutationConditions(setA, setB) && subsumesByPermutation(setA, setB)) {
#if (RECORD_INTERNAL_METRICS == 1)
      metric->Subsumptions++;
#endif
      setB.metadata.marked = true;
      return false;
    } else {
#if (RECORD_INTERNAL_METRICS == 1)
      metric->HasNoPermutation++;
#endif
    }

    if (permutationConditions(setB, setA) && subsumesByPermutation(setB, setA)) {
#if (RECORD_INTERNAL_METRICS == 1)
      metric->Subsumptions++;
#endif
      setA.metadata.marked = true;
      return true;
    } else {
#if (RECORD_INTERNAL_METRICS == 1)
      metric->HasNoPermutation++;
#endif
    }
    return false;
  }

  // mark redundant sets within a file
  template <typename II> constexpr void markRedundantNetworks(II it, const II end) const {
    for (; it != end; ++it) {
      Set& setA{*it};
      if (setA.metadata.marked) {
        continue;
      }

      for (auto it2{it + 1}; it2 != end; ++it2) {
        Set& setB{*it2};
        if (setB.metadata.marked) {
          continue;
        }

        if (marked(setA, setB)) {
          break;
        }
      }
    }
  }

  // mark redundant sets across two files
  template <typename II, typename IIMut>
  constexpr void markRedundantNetworks(const II begin1, const II end1, const IIMut begin2,
                                       const IIMut end2) const {
    for (II it1{begin1}; it1 != end1; ++it1) {
      const Set& setA{*it1};
      if (setA.metadata.marked) {
        continue;
      }

      for (IIMut it2{begin2}; it2 != end2; ++it2) {
        Set& setB{*it2};
        if (setB.metadata.marked) {
          continue;
        }

        if (!(permutationConditions(setA, setB) && subsumesByPermutation(setA, setB))) {
#if (RECORD_INTERNAL_METRICS == 1)
          metric->HasNoPermutation++;
#endif
          continue;
        }

#if (RECORD_INTERNAL_METRICS == 1)
        metric->Subsumptions++;
#endif
        setB.metadata.marked = true;
      }
    }
  }

public:
  GenerateAndPrune() : pool(NrOfCores) {}
  ::sortnet::MetricsLayered<N, K> run() {
    metric = &metrics.at(0);

#if (RECORD_IO_TIME == 1)
    auto nanosecondsToSeconds = [](const double_t ns) { return ns / 1000000000.0; };
#endif

    auto microsecondsToSeconds = [](const double_t mcs) { return mcs / 1000000.0; };
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
    uint8_t layer{0};
    storeEmptyNetworkWithOutputSet();
    for (layer = 1; layer <= K; ++layer) {
      metric = &metrics.at(layer);

      // layer insight / progress
#if (PRINT_LAYER_SUMMARY == 1) or (PRINT_PROGRESS == 1)
      std::cout << std::endl
                << "======== N" << int(N) << "K" << int(layer)
                << " =========================================="
                << "===========================================" << std::endl;
#endif

      uint64_t generated{0};
      {  // GENERATE NETWORKS AND OUTPUT SETS
#if (RECORD_INTERNAL_METRICS == 1)
        const auto start = now();
#endif
        generated += generate(layer);
#if (RECORD_INTERNAL_METRICS == 1)
        metric->DurationGenerating = duration(start, now());
#endif
      }

#if (RECORD_IO_TIME == 1)
      const auto fileIODurationGen = nanosecondsToSeconds(storage.duration);
#endif

      uint64_t pruned{0};
      {  // PRUNE WITHIN FILES
#if (RECORD_INTERNAL_METRICS == 1)
        const auto start = now();
#endif
        pruned += pruneWithinFiles(layer);
#if (RECORD_INTERNAL_METRICS == 1)
        const auto end = now();
        const auto d = duration(start, end);

        metric->prunedWithinFile = pruned;

        metric->durationPruningWithinFile = d;
        metric->DurationPruning += d;
#endif
      }

      {  // PRUNE ACROSS FILES
#if (RECORD_INTERNAL_METRICS == 1)
        const auto start = now();
#endif
        pruned += pruneAcrossFiles(layer);
#if (RECORD_INTERNAL_METRICS == 1)
        const auto end = now();
        const auto d = duration(start, end);

        metric->prunedAcrossClusters = pruned;

        metric->durationPruningAcrossClusters = d;
        metric->DurationPruning += d;
#endif
      }

      // always record the nr of pruned and generated networks
      metric->Generated = generated;
      metric->Pruned += pruned;

#if (RECORD_IO_TIME == 1)
      const auto fileIODuration = nanosecondsToSeconds(storage.duration);
      storage.duration = 0;

#  if (PRINT_LAYER_SUMMARY == 1)
      printLayerSummary(layer, fileIODurationGen, fileIODuration);
#  endif
#else
#  if (PRINT_LAYER_SUMMARY == 1)
      printLayerSummary(layer, 0, 0);
#  endif
#endif

      // in case the program needs to terminate before we had planned,
      // at least we have the metrics
#if (SAVE_METRICS == 1)
      const auto json = metrics.to_json(NrOfCores, ::sortnet::segment_capacity);
      storage.Save("metrics.json", json);
#endif

      // psuedo detection of sorting network
      if (layer > 1 && generated - pruned == 1) {
        break;
      }
    }

    if (metric->filters() == 1) {
      std::vector<Net> nets(1);
      storage.Load(filenames.front().net, layer, nets.begin(), nets.end());
      const Net& sortingNetwork = nets.at(0);

      ::sortnet::sequence_t s = ::sortnet::sequence::binary::mask<N> & 0b1011010110101011101011;

      std::cout << std::endl;
      std::cout << "======== FOUND SORTING NETWORK FOR N(" + std::to_string(N) + ") ===========================================================";
      std::cout << std::endl
          << ::sortnet::to_string<N>(sortingNetwork) << std::endl
          << ::sortnet::to_string_knuth_diagram(sortingNetwork, N, s) << std::endl;
    }

    return metrics;
  }

  template <typename Functor>
  uint64_t read(const NetAndSetFilename& file, uint8_t layer, Functor _f) {
    constexpr uint32_t FileSize{::sortnet::segment_capacity};

    std::vector<Set> sets(FileSize);
    std::vector<Net> nets(FileSize);

    auto find = [](const uint64_t netID, auto it, const auto end) -> Net {
      for (; it != end; ++it) {
        if (it->id == netID) {
          return static_cast<Net>(*it);
        }
      }

      throw std::logic_error("no network was not found for the given network id");
    };

    const auto nrOfNets = storage.Load(file.net, layer, nets.begin(), nets.end());
    const auto nrOfSets = storage.Load(file.set, layer, sets.begin(), sets.end());
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileRead += 2;
#endif
    sets.resize(nrOfSets);

    for (const auto& set : sets) {
      const auto netID{set.metadata.netID};
      const Net& net = find(netID, nets.begin(), nets.begin() + nrOfNets);
      _f(net, set);
    }

    return nrOfSets;  // nrOfNets contains pruned entities
  }

  uint64_t generate(uint8_t layer) {
    const auto existingFiles = std::move(filenames);
    filenames.clear();  // TODO: redundant?

    auto save = [&](auto itNets, const auto endNets, auto itSets, const auto endSets) -> void {
      const auto netFile = storage.Save(layer, itNets, endNets);
      const auto setFile = storage.Save(layer, itSets, endSets);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileWrite += 2;
#endif

      filenames.emplace_back(NetAndSetFilename{
          .net{netFile},
          .set{setFile},
      });
    };

#if (PRINT_PROGRESS == 1)
    uint64_t filters = metrics.at(layer - 1).filters();
    Progress bar("generating", "files", filters);
    bar.display();
#endif

    std::vector<Net> nets(::sortnet::segment_capacity);
    std::vector<Set> sets(::sortnet::segment_capacity);

    uint64_t counter{0};
    uint64_t idCounter{0};
    Set setBuffer{};
    for (const auto& file : existingFiles) {
      auto nrOfNetworks = this->read(file, layer - 1, [&](const Net& net, const Set& set) {
        for (const ::sortnet::Comparator& c : ::sortnet::comparator::all<N>) {
          if (net.back() == c) {
#if (RECORD_INTERNAL_METRICS == 1)
            metric->RedundantComparatorQuick++;
#endif
            continue;
          }

          // apply new comparator and see if it causes a different output set
          setBuffer.clear();
          for (const ::sortnet::sequence_t s : set) {
            const auto k{std::popcount(s) - 1};
            setBuffer.insert(k, c.apply(s));
          }
          if (setBuffer == set) {
#if (RECORD_INTERNAL_METRICS == 1)
            metric->RedundantComparator++;
#endif
            continue;
          }

          nets.at(counter) = net;
          nets.at(counter).id = idCounter;
          nets.at(counter).push_back(c);
          sets.at(counter) = setBuffer;
          sets.at(counter).metadata.netID = idCounter;
          sets.at(counter).metadata.compute();
          ++counter;
          ++idCounter;

          if (counter == ::sortnet::segment_capacity) {
            save(nets.cbegin(), nets.cend(), sets.cbegin(), sets.cend());
            counter = 0;
          }
        }
      });
#if (PRINT_PROGRESS == 1)
      bar += nrOfNetworks;
      bar.display();
#endif
    }

    // check if there is anything else to write to file
    if (counter > 0) {
      save(nets.cbegin(), nets.cbegin() + counter, sets.cbegin(), sets.cbegin() + counter);
    }
#if (PRINT_PROGRESS == 1)
    bar.done();
#endif

    return idCounter;
  }

  uint64_t pruneWithinFiles(uint8_t layer) {
#if (PRINT_PROGRESS == 1)
    std::mutex m;

    Progress bar("pruning", "within files", filenames.size());
    bar.display();
#endif

    auto updateProgress = [&]() -> void {
#if (PRINT_PROGRESS == 1)
      m.lock();
      // const std::lock_guard<std::mutex> lock(m);
      ++bar;
      bar.display();
      m.unlock();
#endif
    };

    auto prune = [&](const auto& filename) -> uint64_t {
      auto* buffer = buffers.get();
      auto begin = buffer->sets.begin();
      auto end = buffer->sets.end();

      auto size = storage.Load(filename, layer, begin, end);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      const auto originalSize{size};
      end = begin + size;

      markRedundantNetworks(begin, end);
      size = shiftRedundant(begin, end);
      if (size == originalSize) {
        buffers.put(buffer);
        updateProgress();
        return 0;
      }

      // write results to file
      storage.Save(filename, begin, begin + size);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileWrite++;
#endif

      buffers.put(buffer);
      updateProgress();
      return originalSize - size;
    };

    std::vector<std::future<uint64_t>> results{};
    results.reserve(filenames.size());
    for (const auto& file : filenames) {
      results.emplace_back(pool.add(prune, file.set));
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

  uint64_t pruneAcrossFiles(uint8_t layer) {
    auto prune = [&](const std::string& filename, auto begin, auto end) -> uint32_t {
      auto* buffer = buffers.get();
      auto begin2 = buffer->sets.begin();
      auto end2 = buffer->sets.end();

      auto size = storage.Load(filename, layer, begin2, end2);
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      if (size == 0) {
        buffers.put(buffer);
        return 0;
      }
      const auto sizeBeforePruning{size};

      end2 = begin2 + size;
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
    auto& sets = buffer->sets;
    std::vector<std::future<uint32_t>> results{};
    uint64_t pruned{0};
    for (std::size_t i{0}; i < filenames.size(); ++i) {
      const auto& file{filenames.at(i)};
      auto size = storage.Load(file.set, layer, sets.begin(), sets.end());
#if (RECORD_INTERNAL_METRICS == 1)
      metric->FileRead++;
#endif
      if (size == 0) {
        continue;
      }

      for (std::size_t j{0}; j < filenames.size(); ++j) {
        if (j == i) {
          continue;
        }
        const auto& filename{filenames.at(j).set};
        results.emplace_back(pool.add(prune, filename, sets.cbegin(), sets.cbegin() + size));
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
    buffers.put(buffer);

#if (PRINT_PROGRESS == 1)
    bar.done();
#endif
    return pruned;
  }

  void printLayerSummary([[maybe_unused]] uint8_t layer,
                         [[maybe_unused]] double_t fileIODurationGen,
                         [[maybe_unused]] double_t fileIODuration) {
    auto dec = [](const double_t v) -> std::string {
      const auto precision = 2;
      return std::to_string(v).substr(0, std::to_string(v).find(".") + precision + 1);
    };

    tabulate::Table t;
    t.add_row({"", "Time", "Filters", "IO Time"});
    std::size_t columns{4};

    std::string ioTime{"-"};
#if (RECORD_IO_TIME == 1)
    ioTime = dec(fileIODurationGen);
#endif
    t.add_row({"Generating", dec(metric->DurationGenerating) + "s",
               std::to_string(metric->Generated), ioTime});

#if (RECORD_IO_TIME == 1)
    ioTime = "-";
#endif
    t.add_row(
        {"Pruning", dec(metric->DurationPruning) + "s", std::to_string(metric->Pruned), ioTime});

#if (RECORD_IO_TIME == 1)
    ioTime = dec(fileIODuration);
#endif
    t.add_row({"Sum", dec(metric->DurationGenerating + metric->DurationPruning) + "s",
               std::to_string(metric->Generated - metric->Pruned), ioTime});

#if (RECORD_IO_TIME == 1)
    ioTime = "-";
#endif
    t.add_row({"Sum(layers)", dec(metrics.Seconds()) + "s", "-", ioTime});

    for (std::size_t i{1}; i < columns; ++i) {
      t.column(i).format().font_align(tabulate::FontAlign::right);
    }

    // center-align and color header cells
    for (std::size_t i{0}; i < columns; ++i) {
      t[0][i]
          .format()
          .font_color(tabulate::Color::yellow)
          .font_align(tabulate::FontAlign::center)
          .font_style({tabulate::FontStyle::bold});
    }

    std::cout << std::endl << t << std::endl;
  }
};
