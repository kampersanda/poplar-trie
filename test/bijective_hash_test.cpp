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

#include "test_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::test;

constexpr uint64_t N = 1ULL << 10;

template <typename Hasher>
void check_bijection(uint32_t univ_bits) {
    Hasher h{univ_bits};

    if (h.size() <= N) {
        for (uint64_t x = 0; x < h.size(); ++x) {
            ASSERT_EQ(x, h.hash_inv(h.hash(x)));
        }
    } else {
        std::random_device rnd;
        for (uint64_t i = 0; i < N; ++i) {
            uint64_t x = rnd() % N;
            ASSERT_EQ(x, h.hash_inv(h.hash(x)));
        }
    }
}

template <typename>
class bijective_hash_test : public ::testing::Test {};

using bijective_hash_types = ::testing::Types<bijective_hash::split_mix_hasher>;

TYPED_TEST_CASE(bijective_hash_test, bijective_hash_types);

TYPED_TEST(bijective_hash_test, Tiny) {
    for (uint32_t i = 1; i < 64; ++i) {
        check_bijection<TypeParam>(i);
    }
}

}  // namespace
