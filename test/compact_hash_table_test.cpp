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
#include <map>

#include <gtest/gtest.h>
#include <poplar.hpp>

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint32_t VAL_BITS = 16;
constexpr uint64_t VAL_MASK = (1ULL << VAL_BITS) - 1;

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

TEST(compact_hash_table_test, Tiny) {
    const uint64_t univ_bits = 14;
    const uint64_t capa_bits = 8;
    const uint64_t size = 1ULL << (univ_bits - 1);

    auto m = create_map(univ_bits, size);
    compact_hash_table<VAL_BITS> cht(univ_bits, capa_bits);

    for (auto item : m) {
        cht.set(item.first, item.second);
    }

    for (auto item : m) {
        ASSERT_EQ(cht.get(item.first), item.second);
    }
}

}  // namespace
