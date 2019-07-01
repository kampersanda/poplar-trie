/**
 * MIT License
 *
 * Copyright (c) 2018–2019 Shunsuke Kanda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

template <typename Trie>
void insert_keys(Trie& ht, const std::vector<std::string>& keys, std::vector<uint64_t>& ids) {
    ASSERT_FALSE(keys.empty());

    ids.resize(ht.capa_size(), UINT64_MAX);

    ht.add_root();
    auto num_nodes = ht.size();

    if constexpr (Trie::trie_type_id == trie_type_ids::FKHASH_TRIE) {
        ASSERT_EQ(ht.get_root(), 0);
    }

    for (uint64_t i = 0; i < keys.size(); ++i) {
        auto node_id = ht.get_root();

        for (auto c : keys[i]) {
            if (ht.add_child(node_id, static_cast<uint8_t>(c))) {
                if constexpr (Trie::trie_type_id == trie_type_ids::FKHASH_TRIE) {
                    ASSERT_EQ(node_id, num_nodes);
                }

                ++num_nodes;

                if constexpr (Trie::trie_type_id == trie_type_ids::BONSAI_TRIE) {
                    if (!ht.needs_to_expand()) {
                        continue;
                    }
                    auto node_map = ht.expand();
                    node_id = node_map[node_id];
                    std::vector<uint64_t> new_ids(ht.capa_size(), UINT64_MAX);
                    for (uint64_t j = 0; j < node_map.size(); ++j) {
                        if (node_map[j] != UINT64_MAX) {
                            new_ids[node_map[j]] = ids[j];
                        }
                    }
                    ids = std::move(new_ids);
                } else {
                    if (ids.size() < ht.capa_size()) {
                        ids.resize(ht.capa_size());
                    }
                }
            }
        }

        ids[node_id] = i;
    }

    ASSERT_EQ(num_nodes, ht.size());
}

template <typename Trie>
void search_keys(const Trie& ht, const std::vector<std::string>& keys, const std::vector<uint64_t>& ids) {
    ASSERT_FALSE(keys.empty());

    for (uint64_t i = 0; i < keys.size(); ++i) {
        auto node_id = ht.get_root();
        for (auto c : keys[i]) {
            node_id = ht.find_child(node_id, static_cast<uint8_t>(c));
            ASSERT_NE(node_id, Trie::nil_id);
        }

        ASSERT_EQ(i, ids[node_id]);
    }
}

template <typename Trie>
void restore_keys(const Trie& ht, const std::vector<std::string>& keys, const std::vector<uint64_t>& ids) {
    ASSERT_FALSE(keys.empty());

    if constexpr (Trie::trie_type_id == trie_type_ids::BONSAI_TRIE) {
        std::string restore;

        for (uint64_t i = 0; i < ids.size(); ++i) {
            if (ids[i] == UINT64_MAX) {
                continue;
            }

            restore.clear();

            uint64_t node_id = i;
            while (node_id != ht.get_root()) {
                auto ps = ht.get_parent_and_symb(node_id);
                ASSERT_NE(ps.first, Trie::nil_id);
                node_id = ps.first;
                restore += static_cast<char>(ps.second);
            }

            std::reverse(restore.begin(), restore.end());
            ASSERT_EQ(restore, keys[ids[i]]);
        }
    }
}

template <typename>
class hash_trie_test : public ::testing::Test {};

using hash_trie_types =
    ::testing::Types<plain_fkhash_trie<>, plain_bonsai_trie<>, compact_fkhash_trie<>, compact_bonsai_trie<>>;

TYPED_TEST_CASE(hash_trie_test, hash_trie_types);

TYPED_TEST(hash_trie_test, tiny) {
    TypeParam ht{0, 8};
    auto keys = make_tiny_keys();
    std::vector<uint64_t> ids;
    insert_keys(ht, keys, ids);
    search_keys(ht, keys, ids);
    restore_keys(ht, keys, ids);
}

TYPED_TEST(hash_trie_test, words) {
    TypeParam ht{20, 8};
    auto keys = load_keys("words.txt");
    std::vector<uint64_t> ids;
    insert_keys(ht, keys, ids);
    search_keys(ht, keys, ids);
    restore_keys(ht, keys, ids);
}

TYPED_TEST(hash_trie_test, words_ex) {
    TypeParam ht{0, 8};
    auto keys = load_keys("words.txt");
    std::vector<uint64_t> ids;
    insert_keys(ht, keys, ids);
    search_keys(ht, keys, ids);
    restore_keys(ht, keys, ids);
}

}  // namespace
