#include <random>
#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

namespace bijective_hash_test {

constexpr uint64_t N = 1ULL << 10;

template <typename t_hash>
void check_bijection(uint32_t univ_bits) {
  t_hash h{univ_bits};

  if (h.size() <= N) {
    for (uint64_t x = 0; x < h.size(); ++x) {
      ASSERT_EQ(x, h.hash_inv(h.hash(x)));
    }
  } else {
    std::random_device rnd;
    for (uint64_t i = 0; i < N; ++i) {
      uint64_t x = rnd() % N;
      ASSERT_EQ(x, h.hash_inv(h.hash(x)));
    }
  }
}

} // ns - bijective_hash_test

template<typename>
class BijectiveHashTest : public ::testing::Test {};

using BijectiveHashTypes = ::testing::Types<
  bijective_hash::SplitMix
>;

TYPED_TEST_CASE(BijectiveHashTest, BijectiveHashTypes);

TYPED_TEST(BijectiveHashTest, Tiny) {
  for (uint32_t i = 1; i < 64; ++i) {
    bijective_hash_test::check_bijection<TypeParam>(i);
  }
}

} // ns
