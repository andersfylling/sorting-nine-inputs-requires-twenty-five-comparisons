#define LEMMA_7 1
#define RECORD_INTERNAL_METRICS 1
#define PRINT_LAYER_SUMMARY 1
#define PRINT_PROGRESS 1
#define RECORD_ANALYSIS 1
#define SAVE_METRICS 1
#define PREFER_SAFETY 1
#define RECORD_IO_TIME 0

#include <sortnet/networks/Network.h>
#include <sortnet/sets/ListNaive.h>

#include <cxxopts.hpp>
#include <iostream>

#include "GenerateAndPrune.h"
#include "persistentStorage.h"
#include "settings.h"

constexpr uint8_t N{PARAM_N > 0 ? PARAM_N : 7};
constexpr uint8_t K{PARAM_K > 0 ? PARAM_K : ::sortnet::networkSizeUpperBound<N>()};
constexpr uint8_t Threads{PARAM_THREADS > 2 ? PARAM_THREADS : 2};

int main(int argc, char** argv) {
  cxxopts::Options options(argv[0],
                           "Generate and prune approach for finding the smallest sized sorting "
                           "network for a sequence of N elements");

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("info", "Project information")
  ;
  // clang-format on

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>()) {
    std::cout << options.help() << std::endl;
    return 0;
  } else if (result["info"].as<bool>()) {
    std::cout << "Third part c++ implementation of the paper \"Sorting nine inputs requires "
                 "twenty-five comparisons\"."
              << std::endl
              << std::endl;
    std::cout
        << "https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons"
        << std::endl;
    std::cout << "MIT License - Copyright (c) 2020 Anders Øen Fylling" << std::endl;
    return 0;
  }

  std::ios::sync_with_stdio(false);
  using Set = ::sortnet::set::ListNaive<N, K>;
  using Net = ::sortnet::network::Network<N, K>;
  using Storage = PersistentStorage<Net, Set, N, K>;

  auto g = GenerateAndPrune<N, K, Threads - 1, Set, Net, Storage>{};
  const auto m = g.run();

  usleep(500000);  // 0.5s
  return 0;
}