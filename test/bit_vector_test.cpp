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