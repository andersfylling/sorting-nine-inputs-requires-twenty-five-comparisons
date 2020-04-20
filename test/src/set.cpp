#include <doctest/doctest.h>

#include <iostream>
#include <string>

#define UNIT_TEST 1

#include <sortnet/networks/Network.h>
#include <sortnet/permutation.h>
#include <sortnet/sequence.h>
#include <sortnet/sets/ListNaive.h>
#include <sortnet/util.h>
#include <sortnet/z_environment.h>

#include "utilTest.h"

auto removeSpaces
    = [](auto &str) { str.erase(remove_if(str.begin(), str.end(), isspace), str.end()); };

TEST_CASE("Subsuming check") {
  /*
   "For example, consider the networks Ca = (0, 1); (1, 2); (0, 3) and
    Cb = (0, 1); (0, 2); (1, 3). Their output sets are:
    ouputs(C a ) = {{0000}, {0001, 0010}, {0011, 0110}, {0111, 1011}, {1111}},
    ouputs(C b ) = {{0000}, {0001, 0010}, {0011, 0101}, {0111, 1011}, {1111}}.
    It is easy to verify that π = (0, 1, 3, 2) has the property that Ca ≤ π Cb."

   Taken from the paper: "An Improved Subsumption Testing Algorithm for
                          the Optimal-Size Sorting Network Problem"
                          Cristian Frăsinaru and Mădălina Răschip
                          Faculty of Computer Science,
                          ”Alexandru Ioan Cuza” University, Iaşi, Romania
   */
  // In the reference above, they state an example where a comparison of the
  // output sets, without a permutation, will not function. And that given the
  // permutation (0,1,3,2) that network Ca subsumes network Cb.
  //
  // This test uses this as a truth to validate program behaviour.
  constexpr uint8_t N = 4;
  constexpr uint8_t K = 3;
  using set_t = ::sortnet::set::ListNaive<N, K>;
  using net_t = ::sortnet::network::Network<N, K>;

  net_t Ca{};
  // --+-----+--
  //   |     |
  // --+--+-----
  //      |  |
  // -----+-----
  //         |
  // --------+--
  Ca.push_back(comp<N>(0, 1));
  Ca.push_back(comp<N>(1, 2));
  Ca.push_back(comp<N>(0, 3));
  REQUIRE(Ca.size() == K);

  SUBCASE("Network Ca is correct") {
    auto CaStr = ::sortnet::to_string<N>(Ca);
    removeSpaces(CaStr);
    std::string paperStrCa{"(0, 1); (1, 2); (0, 3);"};
    removeSpaces(paperStrCa);
    REQUIRE(paperStrCa == CaStr);
  }

  net_t Cb{};
  // --+--+-----
  //   |  |
  // --+-----+--
  //      |  |
  // -----+-----
  //         |
  // --------+--
  Cb.push_back(comp<N>(0, 1));
  Cb.push_back(comp<N>(0, 2));
  Cb.push_back(comp<N>(1, 3));
  REQUIRE(Cb.size() == 3);

  SUBCASE("Network Cb is correct") {
    std::string CbStr = ::sortnet::to_string<N>(Cb);
    removeSpaces(CbStr);
    std::string paperStrCb{"(0, 1); (0, 2); (1, 3);"};
    removeSpaces(paperStrCb);
    CHECK(paperStrCb == CbStr);
  }

  set_t CaOutputs{};
  populate<N>(Ca, CaOutputs);
  SUBCASE("Network Ca yields the valid sequence set") {
    // ouputs(Ca) = {{0000}, {0001, 0010}, {0011, 0110}, {0111, 1011}, {1111}}
    //
    // this program skips the first and the last sequences as they are
    // redundant. Cleaning up the output yields:
    // ouputs(Ca) = {{0001, 0010}, {0011, 0110}, {0111, 1011}}
    auto CaOutputsStr = ::sortnet::to_string<N>(CaOutputs);
    removeSpaces(CaOutputsStr);

    std::string paperStrCaOutputs{"({0001, 0010}, {0011, 0110}, {0111, 1011})"};
    removeSpaces(paperStrCaOutputs);

    CHECK(paperStrCaOutputs == CaOutputsStr);
  }

  set_t CbOutputs{};
  populate<N>(Cb, CbOutputs);
  SUBCASE("Network Cb yields the valid sequence set") {
    // ouputs(Cb) = {{0000}, {0001, 0010}, {0011, 0101}, {0111, 1011}, {1111}}
    //
    // this program skips the first and the last sequences as they are
    // redundant. Cleaning up the output yields:
    // ouputs(Cb) = {{0001, 0010}, {0011, 0101}, {0111, 1011}}
    auto CbOutputsStr = ::sortnet::to_string<N>(CbOutputs);
    removeSpaces(CbOutputsStr);

    std::string paperStrCbOutputs{"({0001, 0010}, {0011, 0101}, {0111, 1011})"};
    removeSpaces(paperStrCbOutputs);

    CHECK(paperStrCbOutputs == CbOutputsStr);
  }

  SUBCASE("Network Ca should not subsume network Cb without a permutation") {
    REQUIRE_FALSE(CaOutputs.subsumes(CbOutputs));
  }

  SUBCASE("Ca subsumes Cb when permutation is applied") {
    const auto p = perm<N>({0, 1, 3, 2});
    REQUIRE(::sortnet::permutation::to_string<N>(p) == "(0,1,3,2)");
    ::sortnet::permutation::apply<N>(p, CaOutputs);

    auto result = CaOutputs.subsumes(CbOutputs);
    if (!result) {
      std::cout << ::sortnet::to_string<N>(CaOutputs) << std::endl;
      std::cout << ::sortnet::to_string<N>(CbOutputs) << std::endl;
    }
    CHECK(result);
  }
}

TEST_CASE("Subsuming check 2 - random networks") {
  constexpr uint8_t N = 4;
  constexpr uint8_t K = 5;
  using set_t = ::sortnet::set::ListNaive<N, K>;
  using net_t = ::sortnet::network::Network<N, K>;

  net_t Ca{};
  // --+--+--+--------
  //   |  |  |
  // --+-----------+--
  //      |  |     |
  // --------+--+-----
  //      |     |  |
  // -----+-----+--+--
  Ca.push_back(comp<N>(0, 1));
  Ca.push_back(comp<N>(0, 3));
  Ca.push_back(comp<N>(0, 2));
  Ca.push_back(comp<N>(2, 3));
  Ca.push_back(comp<N>(1, 3));
  REQUIRE(Ca.size() == K);

  net_t Cb{};
  // --+--+--+--------
  //   |  |  |
  // --+--------+-----
  //      |  |  |
  // --------+--+--+--
  //      |        |
  // -----+--------+--
  Cb.push_back(comp<N>(0, 1));
  Cb.push_back(comp<N>(0, 3));
  Cb.push_back(comp<N>(0, 2));
  Cb.push_back(comp<N>(1, 2));
  Cb.push_back(comp<N>(2, 3));
  REQUIRE(Cb.size() == K);

  set_t CaOutputs{};
  populate<N>(Ca, CaOutputs);

  set_t CbOutputs{};
  populate<N>(Cb, CbOutputs);

  SUBCASE("Ca subsumes Cb") {
    REQUIRE(CaOutputs.subsumes(CbOutputs));
    const auto p = perm<N>({0, 1, 2, 3});
    ::sortnet::permutation::apply<N>(p, CaOutputs);
    REQUIRE(CaOutputs.subsumes(CbOutputs));
  }
}