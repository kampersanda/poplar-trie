#include <gtest/gtest.h>
#include <poplar.hpp>
#include <random>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint64_t N = 1ULL << 10;

template <typename Hasher>
void check_bijection(uint32_t univ_bits) {
  Hasher h{univ_bits};

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

template <typename>
class bijective_hash_test : public ::testing::Test {};

using bijective_hash_types = ::testing::Types<bijective_hash::split_mix_hasher>;

TYPED_TEST_CASE(bijective_hash_test, bijective_hash_types);

TYPED_TEST(bijective_hash_test, Tiny) {
  for (uint32_t i = 1; i < 64; ++i) {
    check_bijection<TypeParam>(i);
  }
}

}  // namespace
