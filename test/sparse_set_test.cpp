#include <gtest/gtest.h>
#include <poplar.hpp>
#include <random>

#include <poplar/sparse_set.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint64_t N = 10000;

void test_access(uint64_t factor) {
  // std::vector<bool> bits;
  std::vector<uint64_t> selects;
  sparse_set set;

  {
    std::random_device rnd;
    for (uint64_t i = 0; i < N; ++i) {
      uint64_t x = rnd() % 100;
      if (x < factor) {
        selects.push_back(i);
        set.append(i);
      }
    }
  }

  for (uint64_t i = 0; i < selects.size(); ++i) {
    ASSERT_EQ(selects[i], set.access(i));
  }
  for (uint64_t i = 0; i < selects.size() - 1; ++i) {
    auto [s1, s2] = set.access_pair(i);
    ASSERT_EQ(selects[i], s1);
    ASSERT_EQ(selects[i + 1], s2);
  }
}

TEST(sparse_set_test, Tiny1) {
  test_access(1);
}
TEST(sparse_set_test, Tiny10) {
  test_access(10);
}
TEST(sparse_set_test, Tiny50) {
  test_access(50);
}
TEST(sparse_set_test, Tiny90) {
  test_access(90);
}
TEST(sparse_set_test, Tiny99) {
  test_access(99);
}

}  // namespace