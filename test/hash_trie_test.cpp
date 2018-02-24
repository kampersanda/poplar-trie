#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

namespace hash_trie_test {

template <typename t_ht>
void insert_keys(t_ht& ht, const std::vector<std::string>& keys, std::vector<uint64_t>& ids) {
  ids.resize(ht.capa_size(), UINT64_MAX);

  ht.add_root();
  auto num_nodes = ht.size();

  for (uint64_t i = 0; i < keys.size(); ++i) {
    auto node_id = ht.get_root();
    for (auto c : keys[i]) {
      int ret = ht.add_child(node_id, static_cast<uint8_t>(c));
      ASSERT_NE(ret, 2);
      if (ret == 1) {
        ++num_nodes;
      }
    }
    ids[node_id] = i;
  }

  ASSERT_EQ(num_nodes, ht.size());
}

template <typename t_ht>
void insert_keys_ex(t_ht& ht, const std::vector<std::string>& keys, std::vector<uint64_t>& ids) {
  ids.resize(ht.capa_size(), UINT64_MAX);

  ht.add_root();
  auto num_nodes = ht.size();

  for (uint64_t i = 0; i < keys.size(); ++i) {
    auto node_id = ht.get_root();
    for (auto c : keys[i]) {
      int ret = ht.add_child(node_id, static_cast<uint8_t>(c));
      ASSERT_NE(ret, 2);
      if (ret == 1) {
        ++num_nodes;
      }

      if (ht.needs_to_expand()) {
        auto node_map = ht.expand();
        node_id = node_map[node_id];
        std::vector<uint64_t> new_ids(ht.capa_size(), UINT64_MAX);
        for (uint64_t j = 0; j < node_map.size(); ++j) {
          if (node_map[j] != UINT64_MAX) {
            new_ids[node_map[j]] = ids[j];
          }
        }
        ids = std::move(new_ids);
      }
    }
    ids[node_id] = i;
  }

  ASSERT_EQ(num_nodes, ht.size());
}

template <typename t_ht>
void search_keys(const t_ht& ht, const std::vector<std::string>& keys,
                 const std::vector<uint64_t>& ids) {
  for (uint64_t i = 0; i < keys.size(); ++i) {
    auto node_id = ht.get_root();

    for (auto c : keys[i]) {
      node_id = ht.find_child(node_id, static_cast<uint8_t>(c));
      ASSERT_NE(node_id, t_ht::NIL_ID);
    }
    ASSERT_EQ(i, ids[node_id]);
  }
}

template <typename t_ht>
void restore_keys(const t_ht& ht, const std::vector<std::string>& keys,
                  const std::vector<uint64_t>& ids) {
  std::string restore;

  for (uint64_t i = 0; i < ids.size(); ++i) {
    if (ids[i] == UINT64_MAX) {
      continue;
    }

    restore.clear();

    uint64_t node_id = i;
    while (node_id != ht.get_root()) {
      auto ps = ht.get_parent_and_symb(node_id);
      ASSERT_NE(ps.first, t_ht::NIL_ID);
      node_id = ps.first;
      restore += static_cast<char>(ps.second);
    }

    std::reverse(restore.begin(), restore.end());
    ASSERT_EQ(restore, keys[ids[i]]);
  }
}

} // ns - hash_trie_test

template<typename>
class HashTrieTest : public ::testing::Test {};

using HashTrieTypes = ::testing::Types<
  HashTriePR<>, HashTrieCR<>
>;

TYPED_TEST_CASE(HashTrieTest, HashTrieTypes);

TYPED_TEST(HashTrieTest, Tiny) {
  TypeParam ht{0,8};
  auto keys = make_tiny_keys();
  std::vector<uint64_t> ids;
  hash_trie_test::insert_keys(ht, keys, ids);
  hash_trie_test::search_keys(ht, keys, ids);
  hash_trie_test::restore_keys(ht, keys, ids);
}

TYPED_TEST(HashTrieTest, Words) {
  TypeParam ht{20,8};
  auto keys = load_keys("words.txt");
  std::vector<uint64_t> ids;
  hash_trie_test::insert_keys(ht, keys, ids);
  hash_trie_test::search_keys(ht, keys, ids);
  hash_trie_test::restore_keys(ht, keys, ids);
}

TYPED_TEST(HashTrieTest, WordsEx) {
  TypeParam ht{0,8};
  auto keys = load_keys("words.txt");
  std::vector<uint64_t> ids;
  hash_trie_test::insert_keys_ex(ht, keys, ids);
  hash_trie_test::search_keys(ht, keys, ids);
  hash_trie_test::restore_keys(ht, keys, ids);
}

} // ns
