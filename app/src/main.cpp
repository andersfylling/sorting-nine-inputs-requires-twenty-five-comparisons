#include <cxxopts.hpp>
#include <iostream>

#include "GenerateAndPrune.cpp" // TODO: cleanup

int main(int argc, char** argv) {
  cxxopts::Options options(argv[0], "Generate and prune approach for finding the smallest sized sorting network for a sequence of N elements");

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
    std::cout << "Third part c++ implementation of the paper \"Sorting nine inputs requires twenty-five comparisons\"." << std::endl << std::endl;
    std::cout << "https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons" << std::endl;
    std::cout << "MIT License - Copyright (c) 2020 Anders Øen Fylling" << std::endl;
    return 0;
  }

  std::cout << "running" << std::endl;
  return 0;
}
