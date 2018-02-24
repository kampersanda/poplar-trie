#include <map>

#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint32_t VAL_BITS = 16;
constexpr uint64_t VAL_MASK = (1ULL << VAL_BITS) - 1;

namespace compact_hash_table_test {

std::map<uint64_t, uint64_t> create_map(uint32_t univ_bits, uint64_t size) {
  std::map<uint64_t, uint64_t> m;
  std::random_device rnd;

  uint64_t univ_mask = (1ULL << univ_bits) - 1;

  while (m.size() < size) {
    uint64_t key = rnd() & univ_mask;
    uint64_t val = rnd() & VAL_MASK;
    if (val == VAL_MASK) {
      val = 0;
    }
    m.insert(std::make_pair(key, val));
  };

  return m;
};

} // ns - compact_hash_table_test

TEST(CompactHashTableTest, Tiny) {
  const uint64_t univ_bits = 14;
  const uint64_t capa_bits = 8;
  const uint64_t size = 1ULL << (univ_bits - 1);

  auto m = compact_hash_table_test::create_map(univ_bits, size);
  CompactHashTable<VAL_BITS> cht(univ_bits, capa_bits);

  for (auto item : m) {
    cht.set(item.first, item.second);
  }

  for (auto item : m) {
    ASSERT_EQ(cht.get(item.first), item.second);
  }
}

} // ns