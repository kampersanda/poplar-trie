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
#ifndef POPLAR_TRIE_BIT_TOOLS_HPP
#define POPLAR_TRIE_BIT_TOOLS_HPP

#ifdef __SSE4_2__
#include <xmmintrin.h>
#endif

#include "basics.hpp"

namespace poplar::bit_tools {

// Gets a bit
inline bool get_bit(uint8_t x, uint64_t i) {
    assert(i < 8);
    return (x & (1U << i)) != 0;
}
inline bool get_bit(uint16_t x, uint64_t i) {
    assert(i < 16);
    return (x & (1U << i)) != 0;
}
inline bool get_bit(uint32_t x, uint64_t i) {
    assert(i < 32);
    return (x & (1U << i)) != 0;
}
inline bool get_bit(uint64_t x, uint64_t i) {
    assert(i < 64);
    return (x & (1ULL << i)) != 0;
}

// Sets a bit
inline void set_bit(uint8_t& x, uint64_t i, bool bit = true) {
    assert(i < 8);
    if (bit) {
        x |= (1U << i);
    } else {
        x &= ~(1U << i);
    }
}
inline void set_bit(uint16_t& x, uint64_t i, bool bit = true) {
    assert(i < 16);
    if (bit) {
        x |= (1U << i);
    } else {
        x &= ~(1U << i);
    }
}
inline void set_bit(uint32_t& x, uint64_t i, bool bit = true) {
    assert(i < 32);
    if (bit) {
        x |= (1U << i);
    } else {
        x &= ~(1U << i);
    }
}
inline void set_bit(uint64_t& x, uint64_t i, bool bit = true) {
    assert(i < 64);
    if (bit) {
        x |= (1ULL << i);
    } else {
        x &= ~(1ULL << i);
    }
}

constexpr uint8_t POPCNT_TABLE[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2,
    3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
    3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5,
    6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4,
    3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
    5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
    6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

// Popcount
inline uint64_t popcnt(uint8_t x) {
    return POPCNT_TABLE[x];
}
inline uint64_t popcnt(uint16_t x) {
    return POPCNT_TABLE[x & UINT8_MAX] + POPCNT_TABLE[x >> 8];
}
inline uint64_t popcnt(uint32_t x) {
#ifdef __SSE4_2__
    return static_cast<uint64_t>(__builtin_popcount(x));
#else
    x = (x & 0x55555555U) + ((x & 0xAAAAAAAAU) >> 1);
    x = (x & 0x33333333U) + ((x & 0xCCCCCCCCU) >> 2);
    x = (x & 0x0F0F0F0FU) + ((x & 0xF0F0F0F0U) >> 4);
    x *= 0x01010101U;
    return x >> 24;
#endif
}
inline uint64_t popcnt(uint64_t x) {
#ifdef __SSE4_2__
    return static_cast<uint64_t>(__builtin_popcountll(x));
#else
    x = (x & 0x5555555555555555ULL) + ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1);
    x = (x & 0x3333333333333333ULL) + ((x & 0xCCCCCCCCCCCCCCCCULL) >> 2);
    x = (x & 0x0F0F0F0F0F0F0F0FULL) + ((x & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
    x *= 0x0101010101010101ULL;
    return x >> 56;
#endif
}

// Masked Popcount
inline uint64_t popcnt(uint8_t x, uint64_t i) {
    assert(i < 8);
    return popcnt(static_cast<uint8_t>(x & ((1U << i) - 1)));
}
inline uint64_t popcnt(uint16_t x, uint64_t i) {
    assert(i < 16);
    return popcnt(static_cast<uint16_t>(x & ((1U << i) - 1)));
}
inline uint64_t popcnt(uint32_t x, uint64_t i) {
    assert(i < 32);
    return popcnt(static_cast<uint32_t>(x & ((1U << i) - 1)));
}
inline uint64_t popcnt(uint64_t x, uint64_t i) {
    assert(i < 64);
    return popcnt(static_cast<uint64_t>(x & ((1ULL << i) - 1)));
}

// From sdsl-lite (https://github.com/simongog/sdsl-lite)
constexpr uint64_t PS_OVERFLOW[65] = {
    0x8080808080808080ULL, 0x7f7f7f7f7f7f7f7fULL, 0x7e7e7e7e7e7e7e7eULL, 0x7d7d7d7d7d7d7d7dULL, 0x7c7c7c7c7c7c7c7cULL,
    0x7b7b7b7b7b7b7b7bULL, 0x7a7a7a7a7a7a7a7aULL, 0x7979797979797979ULL, 0x7878787878787878ULL, 0x7777777777777777ULL,
    0x7676767676767676ULL, 0x7575757575757575ULL, 0x7474747474747474ULL, 0x7373737373737373ULL, 0x7272727272727272ULL,
    0x7171717171717171ULL, 0x7070707070707070ULL, 0x6f6f6f6f6f6f6f6fULL, 0x6e6e6e6e6e6e6e6eULL, 0x6d6d6d6d6d6d6d6dULL,
    0x6c6c6c6c6c6c6c6cULL, 0x6b6b6b6b6b6b6b6bULL, 0x6a6a6a6a6a6a6a6aULL, 0x6969696969696969ULL, 0x6868686868686868ULL,
    0x6767676767676767ULL, 0x6666666666666666ULL, 0x6565656565656565ULL, 0x6464646464646464ULL, 0x6363636363636363ULL,
    0x6262626262626262ULL, 0x6161616161616161ULL, 0x6060606060606060ULL, 0x5f5f5f5f5f5f5f5fULL, 0x5e5e5e5e5e5e5e5eULL,
    0x5d5d5d5d5d5d5d5dULL, 0x5c5c5c5c5c5c5c5cULL, 0x5b5b5b5b5b5b5b5bULL, 0x5a5a5a5a5a5a5a5aULL, 0x5959595959595959ULL,
    0x5858585858585858ULL, 0x5757575757575757ULL, 0x5656565656565656ULL, 0x5555555555555555ULL, 0x5454545454545454ULL,
    0x5353535353535353ULL, 0x5252525252525252ULL, 0x5151515151515151ULL, 0x5050505050505050ULL, 0x4f4f4f4f4f4f4f4fULL,
    0x4e4e4e4e4e4e4e4eULL, 0x4d4d4d4d4d4d4d4dULL, 0x4c4c4c4c4c4c4c4cULL, 0x4b4b4b4b4b4b4b4bULL, 0x4a4a4a4a4a4a4a4aULL,
    0x4949494949494949ULL, 0x4848484848484848ULL, 0x4747474747474747ULL, 0x4646464646464646ULL, 0x4545454545454545ULL,
    0x4444444444444444ULL, 0x4343434343434343ULL, 0x4242424242424242ULL, 0x4141414141414141ULL, 0x4040404040404040ULL};

// From sdsl-lite (https://github.com/simongog/sdsl-lite)
constexpr uint8_t LT_SEL[256 * 8] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2,
    0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0,
    1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1,
    0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0,
    2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0,
    1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,

    0, 0, 0, 1, 0, 2, 2, 1, 0, 3, 3, 1, 3, 2, 2, 1, 0, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 0, 5, 5, 1, 5,
    2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 0, 6, 6, 1, 6, 2, 2, 1, 6, 3,
    3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2,
    1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 0, 7, 7, 1, 7, 2, 2, 1, 7, 3, 3, 1, 3, 2, 2, 1, 7, 4, 4, 1,
    4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 7, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4,
    3, 3, 1, 3, 2, 2, 1, 7, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2,
    2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,

    0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 3, 3, 2, 0, 0, 0, 4, 0, 4, 4, 2, 0, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 5, 0,
    5, 5, 2, 0, 5, 5, 3, 5, 3, 3, 2, 0, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 6, 0, 6, 6, 2, 0, 6,
    6, 3, 6, 3, 3, 2, 0, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2, 0, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3,
    2, 6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 7, 0, 7, 7, 2, 0, 7, 7, 3, 7, 3, 3, 2, 0, 7, 7, 4,
    7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2, 0, 7, 7, 5, 7, 5, 5, 2, 7, 5, 5, 3, 5, 3, 3, 2, 7, 5, 5, 4, 5, 4, 4, 2, 5,
    4, 4, 3, 4, 3, 3, 2, 0, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2, 7, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3,
    3, 2, 7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 4, 4, 3, 0, 0, 0, 0, 0,
    0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 3, 0, 0, 0, 5, 0, 5, 5, 4, 0, 5, 5, 4, 5, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0,
    0, 6, 0, 6, 6, 3, 0, 0, 0, 6, 0, 6, 6, 4, 0, 6, 6, 4, 6, 4, 4, 3, 0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5,
    3, 0, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 3, 0, 0, 0, 7,
    0, 7, 7, 4, 0, 7, 7, 4, 7, 4, 4, 3, 0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 3, 0, 7, 7, 5, 7, 5, 5, 4, 7,
    5, 5, 4, 5, 4, 4, 3, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 3, 0, 7, 7, 6, 7, 6, 6, 4, 7, 6, 6, 4, 6, 4,
    4, 3, 0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 3, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 4, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6,
    5, 0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0,
    0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5, 0, 0, 0, 7, 0, 7, 7, 5, 0,
    7, 7, 5, 7, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6,
    6, 4, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5, 0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0,
    0, 0, 7, 0, 7, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7,
    7, 6, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};

// From sdsl-lite (https://github.com/simongog/sdsl-lite)
constexpr uint64_t select(uint64_t x, uint64_t i) {
    assert(i != 0);
#ifdef __SSE4_2__
    uint64_t s = x;
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = 0x0101010101010101ULL * s;
    uint64_t b = (s + PS_OVERFLOW[i]) & 0x8080808080808080ULL;
    int byte_nr = __builtin_ctzll(b) >> 3;  // byte nr in [0..7]
    s <<= 8;
    i -= (s >> (byte_nr << 3)) & 0xFFULL;
    return (byte_nr << 3) + LT_SEL[((i - 1) << 8) + ((x >> (byte_nr << 3)) & 0xFFULL)];
#else
    uint64_t s = x;  // s = sum
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = 0x0101010101010101ULL * s;
    uint64_t b = (s + PS_OVERFLOW[i]);  //&0x8080808080808080ULL;// add something to the partial sums to cause overflow
    i = (i - 1) << 8;
    if (b & 0x0000000080000000ULL)  // byte <=3
        if (b & 0x0000000000008000ULL)  // byte <= 1
            if (b & 0x0000000000000080ULL)
                return LT_SEL[(x & 0xFFULL) + i];
            else
                return 8 + LT_SEL[(((x >> 8) & 0xFFULL) + i - ((s & 0xFFULL) << 8)) & 0x7FFULL];  // byte 1;
        else  // byte >1
            if (b & 0x0000000000800000ULL)  // byte <=2
            return 16 + LT_SEL[(((x >> 16) & 0xFFULL) + i - (s & 0xFF00ULL)) & 0x7FFULL];  // byte 2;
        else
            return 24 + LT_SEL[(((x >> 24) & 0xFFULL) + i - ((s >> 8) & 0xFF00ULL)) & 0x7FFULL];  // byte 3;
    else  //  byte > 3
        if (b & 0x0000800000000000ULL)  // byte <=5
        if (b & 0x0000008000000000ULL)  // byte <=4
            return 32 + LT_SEL[(((x >> 32) & 0xFFULL) + i - ((s >> 16) & 0xFF00ULL)) & 0x7FFULL];  // byte 4;
        else
            return 40 + LT_SEL[(((x >> 40) & 0xFFULL) + i - ((s >> 24) & 0xFF00ULL)) & 0x7FFULL];  // byte 5;
    else  // byte >5
        if (b & 0x0080000000000000ULL)  // byte<=6
        return 48 + LT_SEL[(((x >> 48) & 0xFFULL) + i - ((s >> 32) & 0xFF00ULL)) & 0x7FFULL];  // byte 6;
    else
        return 56 + LT_SEL[(((x >> 56) & 0xFFULL) + i - ((s >> 40) & 0xFF00ULL)) & 0x7FFULL];  // byte 7;
    return 0;
#endif
}

#ifndef __SSE4_2__
constexpr uint32_t MSB_TABLE[256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
#endif

constexpr uint32_t msb(uint64_t x) {
#ifdef __SSE4_2__
    return x == 0 ? 0 : 63 - __builtin_clzll(x);
#else
    uint64_t x1 = x >> 32;
    if (x1 > 0) {  // 32 < |x| <= 64
        uint64_t x2 = x1 >> 16;
        if (x2 > 0) {  // 48 < |x| <= 64
            x1 = x2 >> 8;
            if (x1 > 0) {  // 56 < |x| <= 64
                return MSB_TABLE[x1] + 56;
            } else {  // 48 < |x| <= 56
                return MSB_TABLE[x2] + 48;
            }
        } else {  // 32 < |x| <= 48
            x2 = x1 >> 8;
            if (x2 > 0) {  // 40 < |x| <= 48
                return MSB_TABLE[x2] + 40;
            } else {  // 32 < |x| <= 40
                return MSB_TABLE[x1] + 32;
            }
        }
    } else {  // 0 < |x| <= 32
        uint64_t x2 = x >> 16;
        if (x2 > 0) {  // 16 < |x| <= 32
            x1 = x2 >> 8;
            if (x1 > 0) {  // 24 < |x| <= 32
                return MSB_TABLE[x1] + 24;
            } else {  // 16 < |x| <= 24
                return MSB_TABLE[x2] + 16;
            }
        } else {  // 0 < |x| <= 16
            x1 = x >> 8;
            if (x1 > 0) {  // 8 < |x| <= 16
                return MSB_TABLE[x1] + 8;
            } else {  // 0 < |x| <= 8
                return MSB_TABLE[x];
            }
        }
    }
#endif
}

constexpr uint32_t ceil_log2(uint64_t x) {
    return (x > 1) ? msb(x - 1) + 1 : 0;
}

template <typename T = uint64_t>
constexpr uint64_t words_for(uint64_t bits) {
    uint64_t word_bits = sizeof(T) * 8;
    return (bits + word_bits - 1) / word_bits;
}

}  // namespace poplar::bit_tools

#endif  // POPLAR_TRIE_BIT_TOOLS_HPP
