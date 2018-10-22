#include <gtest/gtest.h>
#include <poplar.hpp>
#include <random>

#include <poplar/sparse_set.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint64_t N = 10000;

void test_select(uint64_t factor) {
  std::vector<bool> bits;
  std::vector<uint64_t> selects;
  sparse_set set;

  {
    std::random_device rnd;
    for (uint64_t i = 0; i < N; ++i) {
      uint64_t x = rnd() % 100;
      if (x < factor) {
        bits.push_back(true);
        set.append(true);
        selects.push_back(i);
      } else {
        bits.push_back(false);
        set.append(false);
      }
    }
  }

  for (uint64_t i = 0; i < selects.size(); ++i) {
    ASSERT_EQ(selects[i], set.select(i));
  }
}

TEST(sparse_set_test, Tiny10) {
  test_select(10);
}
TEST(sparse_set_test, Tiny20) {
  test_select(20);
}
TEST(sparse_set_test, Tiny50) {
  test_select(50);
}

}  // namespace