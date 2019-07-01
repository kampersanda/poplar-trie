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

using value_type = uint64_t;

template <typename Map>
void insert_keys(Map& map, const std::vector<std::string>& keys) {
    ASSERT_FALSE(keys.empty());

    uint64_t num_keys = 0;
    for (uint64_t i = 0; i < keys.size(); i += 2) {
        auto ptr = map.update(make_char_range(keys[i]));
        ASSERT_EQ(*ptr, 0);
        *ptr = i;
        ++num_keys;
    }

    ASSERT_EQ(map.size(), num_keys);
}

template <typename Map>
void search_keys(Map& map, const std::vector<std::string>& keys) {
    ASSERT_FALSE(keys.empty());

    for (uint64_t i = 0; i < keys.size(); i += 2) {
        auto ptr = map.find(make_char_range(keys[i]));
        ASSERT_NE(ptr, nullptr);
        ASSERT_EQ(*ptr, i);
    }

    for (uint64_t i = 0; i < keys.size(); i += 2) {
        auto ptr = map.update(make_char_range(keys[i]));
        ASSERT_NE(ptr, nullptr);
        ASSERT_EQ(*ptr, i);
    }

    for (uint64_t i = 1; i < keys.size(); i += 2) {
        auto ptr = map.find(make_char_range(keys[i]));
        ASSERT_EQ(ptr, nullptr);
    }
}

// clang-format off
using map_types = ::testing::Types<plain_bonsai_map<value_type>,
                                   compact_bonsai_map<value_type>,
                                   plain_fkhash_map<value_type>,
                                   compact_fkhash_map<value_type>
                                   >;
// clang-format on

template <typename>
class map_test : public ::testing::Test {};

TYPED_TEST_CASE(map_test, map_types);

TYPED_TEST(map_test, Tiny) {
    TypeParam map;
    auto keys = make_tiny_keys();
    insert_keys(map, keys);
    search_keys(map, keys);
}

TYPED_TEST(map_test, Words) {
    TypeParam map;
    auto keys = load_keys("words.txt");
    insert_keys(map, keys);
    search_keys(map, keys);
}

}  // namespace
