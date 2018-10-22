#include <gtest/gtest.h>
#include <poplar.hpp>
#include <random>

#include <poplar/bit_vector.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint64_t N = 10000;

TEST(bit_vector_test, Tiny) {
  std::vector<uint64_t> orig;
  bit_vector bv;

  {
    std::random_device rnd;
    for (uint64_t i = 0; i < N; ++i) {
      uint64_t x = rnd() & UINT32_MAX;
      orig.push_back(x);
      bv.append_bits(x, bit_tools::ceil_log2(x));
    }
  }

  uint64_t pos = 0;
  for (uint64_t i = 0; i < N; ++i) {
    uint64_t len = bit_tools::ceil_log2(orig[i]);
    ASSERT_EQ(orig[i], bv.get_bits(pos, len));
    pos += len;
  }
}

}  // namespace