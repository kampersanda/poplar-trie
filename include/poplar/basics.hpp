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
#ifndef POPLAR_TRIE_BASICS_HPP
#define POPLAR_TRIE_BASICS_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "poplar_config.hpp"

namespace poplar {

using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

enum class trie_type_ids : uint8_t { BONSAI_TRIE, FKHASH_TRIE };

struct char_range {
    const uint8_t* begin = nullptr;
    const uint8_t* end = nullptr;

    uint8_t operator[](uint64_t i) const {
        return begin[i];
    }
    bool empty() const {
        return begin == end;
    }
    uint64_t length() const {
        return static_cast<uint64_t>(end - begin);
    }
};

inline char_range make_char_range(const char* str) {
    auto ptr = reinterpret_cast<const uint8_t*>(str);
    return {ptr, ptr + (std::strlen(str) + 1)};
}
inline char_range make_char_range(const std::string& str) {
    auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());
    return {ptr, ptr + (str.size() + 1)};
}

constexpr bool is_power2(uint64_t n) {
    return n != 0 and (n & (n - 1)) == 0;
}

constexpr uint32_t bits_to_bytes(uint32_t bits) {
    if (bits == 0) {
        return 0;
    }
    return bits / 8 + (bits % 8 == 0 ? 0 : 1);
}

constexpr void copy_bytes(uint8_t* dst, const uint8_t* src, uint64_t num) {
    for (uint64_t i = 0; i < num; ++i) {
        dst[i] = src[i];
    }
}

// <quo, mod>
template <uint64_t N>
constexpr std::pair<uint64_t, uint64_t> decompose_value(uint64_t x) {
    return {x / N, x % N};
}

class size_p2 {
  public:
    size_p2() = default;

    explicit size_p2(uint32_t bits) : bits_{bits}, mask_{(1ULL << bits) - 1} {}

    uint32_t bits() const {
        return bits_;
    }
    uint64_t mask() const {
        return mask_;
    }
    uint64_t size() const {
        return mask_ + 1;
    }

  private:
    uint32_t bits_ = 0;
    uint64_t mask_ = 0;
};

inline std::string get_indent(int n) {
    return std::string(n * 4, ' ');
}
inline void show_member(std::ostream& os, const std::string& indent, const char* k) {
    os << indent << k << ":\n";
}
template <class V>
inline void show_stat(std::ostream& os, const std::string& indent, const char* k, const V& v) {
    os << indent << k << ':' << v << '\n';
}

template <uint64_t N>
struct chunk_type_traits;
template <>
struct chunk_type_traits<8> {
    using type = uint8_t;
};
template <>
struct chunk_type_traits<16> {
    using type = uint16_t;
};
template <>
struct chunk_type_traits<32> {
    using type = uint32_t;
};
template <>
struct chunk_type_traits<64> {
    using type = uint64_t;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_BASICS_HPP
