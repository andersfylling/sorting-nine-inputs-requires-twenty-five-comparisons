#pragma once

#include <array>
#include <bit>
#include <iostream>
#include <list>
#include <vector>

#include <sortnet/BufferPool.h>
#include <sortnet/json.h>
#include <sortnet/metric.h>
#include <sortnet/concepts.h>
#include <sortnet/permutation.h>
#include <sortnet/vendors/github.com/dabbertorres/ThreadPool/ThreadPool.h>

#include <sortnet/z_environment.h>

struct NetAndSetFilename {
  const std::string net;
  const std::string set;
};

template <std::size_t limit, ::sortnet::concepts::Set Set, ::sortnet::concepts::ComparatorNetwork Net>
class NetAndSet {
 public:
  std::size_t      size{0};
  std::vector<Set> sets{};
  std::vector<Net> nets{};


  using const_iterator = typename std::vector<Set>::const_iterator;

  NetAndSet() : sets(limit), nets(limit) {}

  const_iterator cend() const noexcept {
    return sets.cend();
  }

  const_iterator add(const Net &net, const Set &set) {
    sets.at(size) = set;
    nets.at(size) = net;
    ++size;

    return sets.cbegin() + size;
  }

  void clear() {
    size = 0;
    // skipping clear for each net and set, to allow
    // re-use of memory
  }
};

template <uint8_t N, uint8_t K, uint8_t NrOfCores, ::sortnet::concepts::Set Set, ::sortnet::concepts::ComparatorNetwork Net,
          typename Storage>
class GenerateAndPrune {
 private:
  static const auto           FileLimit{::sortnet::segment_capacity * 2};  // ugly
  std::vector<NetAndSetFilename> filenames{};
  Storage storage{};

  ::sortnet::BufferPool<Set, Net, NrOfCores, ::sortnet::segment_capacity> buffers{};

  ::sortnet::MetricsLayered<N, K> metrics{};
  ::sortnet::MetricLayer *        metric = &metrics.at(0);

  ::dbr::cc::ThreadPool pool;

  constexpr void storeEmptyNetworkWithOutputSet() {
    // we store an empty network and a complete output set
    std::array<Net, 1> _networks{};
    auto& net{_networks.at(0)};
    std::array<Set, 1> _sets{};
    auto& set{_sets.at(0)};

    // cleanup
    net.clear();
    set.reset();

    // create relationship
    net.id = 1;
    set.metadata.netID = net.id;

    // populate the output set with all the sequences from <0, N^2]
    for (::sortnet::sequence_t s{1}; s <= ::sortnet::sequence::binary::size<N>(); ++s) {
      const auto k{std::popcount(s) - 1};
      set.insert(k, s);
    }
    set.computeMeta(); // compute metadata, if needed...

    // store
    const auto layer{0};
    const auto netFile = storage.Save(layer, _networks.cbegin(), _networks.cend());
    const auto setFile = storage.Save(layer, _sets.cbegin(), _sets.cend());
#if (RECORD_INTERNAL_METRICS == 1)
    metric->FileWrite++;
#endif

    filenames.emplace_back(NetAndSetFilename{
        .net{netFile},
        .set{setFile},
    });
  }

  // group together the unmarked sets
  template <typename _II, typename _II2>
  constexpr uint64_t shiftRedundant(_II it, const _II2 end) {
    // prune away marked sets by shifting those of interest
    // to the left and decreasing the size.
    auto counter{0};
    bool skewed{false};
    _II  it2 = it;
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

  constexpr bool subsumesByPermutation(const Set &setA, const Set &setB) {
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
        constraints,
        [&](const ::sortnet::permutation::permutation_t<N> &p) {
#if (RECORD_INTERNAL_METRICS == 1)
          metric->Permutations++;
#endif
          return ::sortnet::permutation::subsumes<N>(p, setA, setB);
        });
  }

  constexpr bool permutationConditions(const Set &setA, const Set &setB) {
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
  constexpr bool marked(Set &setA, Set &setB) const {
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
  }

  // mark redundant sets within a file
  template<typename II>
  constexpr void markRedundantNetworks(II it, const II end) const {
    for (; it != end; ++it) {
      Set setA{*it};
      if (setA.metadata.marked) {
        continue;
      }

      for (auto it2{it + 1}; it2 != end; ++it2) {
        Set setB{*it2};
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
  template<typename II>
  constexpr void markRedundantNetworks(const II begin1, const II end1, const II begin2, const II end2) const {
    for (II it1{begin1}; it1 != end1; ++it1) {
      Set setA{*it1};
      if (setA.metadata.marked) {
        continue;
      }

      for (II it2{begin2}; it2 != end2; ++it2) {
        Set setB{*it2};
        if (setB.metadata.marked) {
          continue;
        }

        if (marked(setA, setB)) {
          break;
        }
      }
    }
  }

 public:
  GenerateAndPrune() : pool(NrOfCores) {}
  ::sortnet::MetricsLayered<N, K> run();

  uint64_t generate(uint8_t layer);

  uint64_t pruneWithinFiles(uint8_t layer);
  uint64_t pruneAcrossFiles(uint8_t layer);

  void printLayerSummary(uint8_t layer, double_t fileIODurationGen, double_t fileIODuration);
};
