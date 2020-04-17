#include <doctest/doctest.h>
#include <bitset>

#define UNIT_TEST 1

#include <sortnet/concepts.h>
#include <sortnet/networks/Network.h>
#include <sortnet/sequence.h>
#include <sortnet/sets/ListNaive.h>

#include "sortnet/permutation.h"
#include "sortnet/util.h"
#include "utilTest.h"

template <uint8_t N>
using permutation_t = ::sortnet::permutation::permutation_t<N>;
using sequence_t = ::sortnet::sequence_t;

TEST_CASE("apply a permutation to a sequence") {
  constexpr uint8_t N{11};

  const auto p{perm<N>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10})};

  auto max = std::numeric_limits<sequence_t>::max() & ::sortnet::sequence::binary::mask<N>;
  for (sequence_t s{1}; s < max; s++) {
    REQUIRE(s == ::sortnet::permutation::apply<N>(p, s));
  }

  const auto p2{perm<4>({0, 1, 3, 2})};
  const auto permuted = ::sortnet::permutation::apply<4>(p2, 0b0110);
  REQUIRE(0b0101 == permuted);
}

TEST_CASE("incorrect permutation of outputs") {
  constexpr uint8_t N{5};
  constexpr uint8_t K{5};

  static_assert(N == 5);
  using net_t = ::sortnet::network::Network<N, K>;
  using set_t = ::sortnet::set::ListNaive<N, K>;

  net_t A{};
  // (1,2); (3,4); (1,3); (2,4); (2,3);
  A.push_back(comp<N>(1, 2));
  A.push_back(comp<N>(3, 4));
  A.push_back(comp<N>(1, 3));
  A.push_back(comp<N>(2, 4));
  A.push_back(comp<N>(2, 3));

  net_t B{};
  // (1,2); (3,4); (1,3); (2,4); (0,1);
  B.push_back(comp<N>(1, 2));
  B.push_back(comp<N>(3, 4));
  B.push_back(comp<N>(1, 3));
  B.push_back(comp<N>(2, 4));
  B.push_back(comp<N>(0, 1));

  set_t Oa{};
  set_t Ob{};

  constexpr sequence_t mask{::sortnet::sequence::binary::genMask<N>()};
  constexpr auto       max = std::numeric_limits<sequence_t>::max() & mask;
  for (sequence_t s{1}; s < max; s++) {
    const auto k{std::popcount(s) - 1};
    Oa.insert(k, A.run(s));
    Ob.insert(k, B.run(s));
  }

  REQUIRE_FALSE(Oa.subsumes(Ob));

  const auto p{perm<N>({1, 0, 2, 3, 4})};
  CHECK_FALSE(::sortnet::permutation::subsumes<N>(p, Oa, Ob));
}

TEST_CASE("apply permutation") {
  constexpr uint8_t N{5};

  const sequence_t s{0b10111};
  const auto       p{perm<N>({1, 0, 2, 3, 4})};
  const sequence_t permuted = ::sortnet::permutation::apply<N>(p, s);

  REQUIRE_FALSE(s == permuted);
  REQUIRE_FALSE(isSorted<N>(s));
  REQUIRE(isSorted<N>(permuted));
}

TEST_CASE("find permutations given constraints") {
  constexpr uint8_t N{5};

  // given the two networks
  //  Ca = (0,1); (2,3); (1,3); (1,4)
  //  Cb = (0,1); (2,3); (0,3); (1,4)
  // the following partition masks are created
  //  zeros(Ca) = {00000,00000,000-0,000--,000--,-----}
  //  zeros(Cb) = {00000,00000,00000,000--,000--,-----}
  //  ones(Ca)  = {-----,---11,1-111,11111,11111,11111}
  //  ones(Cb)  = {-----,---11,-1111,11111,11111,11111}
  //
  // from these we can derive the following permutation constraints
  std::array<sequence_t, N> constraints{
      0b01100,
      0b11100,  // 0b11100 => 0b10000, due to neighbour entries
      0b01100, 0b00011, 0b00011,
  };

  std::vector<::sortnet::permutation::permutation_t<N>> got{};
  ::sortnet::permutation::generate<N>(
      constraints,
      [&](const auto &p) -> bool {
        got.push_back(p);
        return false;
      });
  const std::vector<::sortnet::permutation::permutation_t<N>> wants{
      {2, 4, 3, 1, 0},
      {2, 4, 3, 0, 1},
      {3, 4, 2, 1, 0},
      {3, 4, 2, 0, 1},
  };

  REQUIRE(got.size() == wants.size());
  for (const auto &p : wants) {
    REQUIRE(std::find(got.cbegin(), got.cend(), p) != got.cend());
  }
}

TEST_CASE("subsumes by perfect matching on partition sets") {
  constexpr uint8_t N{4};
  constexpr uint8_t K{5};

  using set_t = ::sortnet::set::ListNaive<N, K>;

  set_t A{};
  // ({0001,0010,0100},{0011,0101,0110,1100},{0111,1101,1110})
  A.insert(0, 0b0001);
  A.insert(0, 0b0010);
  A.insert(0, 0b0100);
  A.insert(1, 0b0011);
  A.insert(1, 0b0101);
  A.insert(1, 0b0110);
  A.insert(1, 0b1100);
  A.insert(2, 0b0111);
  A.insert(2, 0b1101);
  A.insert(2, 0b1110);
  A.computeMeta();

  set_t B{};
  // ({0001,0010,1000},{0011,0110,1001,1010},{0111,1011,1110})
  B.insert(0, 0b0001);
  B.insert(0, 0b0010);
  B.insert(0, 0b1000);
  B.insert(1, 0b0011);
  B.insert(1, 0b0110);
  B.insert(1, 0b1001);
  B.insert(1, 0b1010);
  B.insert(2, 0b0111);
  B.insert(2, 0b1011);
  B.insert(2, 0b1110);
  B.computeMeta();

  REQUIRE_FALSE(A.subsumes(B));

  ::sortnet::permutation::constraints_t<N> constraints{};
  ::sortnet::permutation::clear<N>(constraints);
  ::sortnet::permutation::constraints<N>(constraints, A, B);
  REQUIRE(::sortnet::permutation::valid_fast<N>(constraints));

  // the permutation (2, 0, 1, 3) should cause this to subsume
  // note that this permutation looks at the sequences
  // from left to right.
  const bool success = ::sortnet::permutation::generate<N>(
      constraints,
      [&](const ::sortnet::permutation::permutation_t<N> &p) {
        return ::sortnet::permutation::subsumes<N>(p, A, B);
      });
      REQUIRE(success);
}