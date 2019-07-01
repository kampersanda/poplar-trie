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
#ifndef POPLAR_TRIE_VBYTE_HPP
#define POPLAR_TRIE_VBYTE_HPP

#include <vector>

#include "basics.hpp"

namespace poplar::vbyte {

inline uint64_t size(uint64_t val) {
    uint64_t n = 1;
    while (127ULL < val) {
        ++n;
        val >>= 7;
    }
    return n;
}

inline uint64_t append(std::vector<uint8_t>& vec, uint64_t val) {
    uint64_t size = vec.size();
    while (127ULL < val) {
        vec.emplace_back(static_cast<uint8_t>((val & 127ULL) | 0x80ULL));
        val >>= 7;
    }
    vec.emplace_back(static_cast<uint8_t>(val & 127ULL));
    return vec.size() - size;
}

inline uint64_t encode(uint8_t* codes, uint64_t val) {
    uint64_t i = 0;
    while (127ULL < val) {
        codes[i++] = static_cast<uint8_t>((val & 127ULL) | 0x80ULL);
        val >>= 7;
    }
    codes[i++] = static_cast<uint8_t>(val & 127ULL);
    return i;
}

inline uint64_t decode(const uint8_t* codes, uint64_t& val) {
    val = 0;
    uint64_t i = 0, shift = 0;
    while ((codes[i] & 0x80) != 0) {
        val |= (codes[i++] & 127ULL) << shift;
        shift += 7;
    }
    val |= (codes[i++] & 127ULL) << shift;
    return i;
}

}  // namespace poplar::vbyte

#endif  // POPLAR_TRIE_VBYTE_HPP
