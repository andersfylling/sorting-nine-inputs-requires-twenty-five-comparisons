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

TEST_CASE("network formatting") {
  constexpr uint8_t N = 4;
  constexpr uint8_t K = 3;
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
}

TEST_CASE("network operations") {
  constexpr uint8_t N = 4;
  constexpr uint8_t K = 5;
  using net_t = ::sortnet::network::Network<N, K>;

  net_t net{};
  REQUIRE(net.empty());

  const ::sortnet::Comparator c{1, 0};
  net.push_back(c);
  REQUIRE(net.size() == 1);

  const auto c2 = net.back();
  REQUIRE(c == c2);

  net.pop_back();
  REQUIRE(net.empty());
  REQUIRE(net.size() == 0);

  // back should return a empty comparator
  REQUIRE(net.back() == ::sortnet::Comparator{0, 0});
}