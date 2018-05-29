#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

using value_type = uint64_t;

namespace map_test {

template <typename t_map>
void insert_keys(t_map& map, const std::vector<std::string>& keys) {
  uint64_t num_keys = 0;
  for (uint64_t i = 0; i < keys.size(); i += 2) {
    auto ptr = map.update(keys[i]);
    ASSERT_EQ(*ptr, 0);
    *ptr = i;
    ++num_keys;
  }

  ASSERT_EQ(map.size(), num_keys);
}

template <typename t_map>
void search_keys(t_map& map, const std::vector<std::string>& keys) {
  for (uint64_t i = 0; i < keys.size(); i += 2) {
    auto ptr = map.find(keys[i]);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(*ptr, i);
  }

  for (uint64_t i = 0; i < keys.size(); i += 2) {
    auto ptr = map.update(keys[i]);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(*ptr, i);
  }

  for (uint64_t i = 1; i < keys.size(); i += 2) {
    auto ptr = map.find(keys[i]);
    ASSERT_EQ(ptr, nullptr);
  }
}

}  // namespace map_test

using MapTypes = ::testing::Types<MapPP<value_type>, MapPE<value_type>, MapPG<value_type>,
                                  MapCP<value_type>, MapCE<value_type>, MapCG<value_type> >;

template <typename>
class MapTest : public ::testing::Test {};

TYPED_TEST_CASE(MapTest, MapTypes);

TYPED_TEST(MapTest, Tiny) {
  TypeParam map;
  auto keys = make_tiny_keys();
  map_test::insert_keys(map, keys);
  map_test::search_keys(map, keys);
}

TYPED_TEST(MapTest, Words) {
  TypeParam map;
  auto keys = load_keys("words.txt");
  map_test::insert_keys(map, keys);
  map_test::search_keys(map, keys);
}

}  // namespace
