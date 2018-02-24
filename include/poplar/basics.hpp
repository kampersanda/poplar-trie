#ifndef POPLAR_TRIE_BASICS_HPP
#define POPLAR_TRIE_BASICS_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

#include "poplar_config.hpp"

namespace poplar {

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

#ifdef POPLAR_ENABLE_EX_STATS
  #define POPLAR_EX_STATS(x) x
#else
  #define POPLAR_EX_STATS(x)
#endif

using ustr_view = std::basic_string_view<uint8_t>;

inline ustr_view make_ustr_view(const char* str) {
  return {reinterpret_cast<const uint8_t*>(str), std::strlen(str) + 1};
}
inline ustr_view make_ustr_view(const char* str, uint64_t len) {
  return {reinterpret_cast<const uint8_t*>(str), len + 1};
}
inline ustr_view make_ustr_view(const std::string& str) {
  auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());
  return {ptr, str.size() + 1};
}
inline ustr_view make_ustr_view(const std::string_view& str) {
  auto ptr = reinterpret_cast<const uint8_t*>(str.data());
  return {ptr, str.size() + 1};
}

constexpr bool is_power2(uint64_t n) {
  return n != 0 && (n & (n - 1)) == 0;
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

struct decomp_val_t {
  uint64_t quo;
  uint64_t mod;
};

template <uint64_t N>
constexpr decomp_val_t decompose_value(uint64_t x) {
  return {x / N, x % N};
}

class size_p2_t {
public:
  size_p2_t() = default;

  explicit size_p2_t(uint32_t bits)
    : bits_{bits}, mask_{(1ULL << bits) - 1} {}

  uint32_t bits() const { return bits_; }
  uint64_t mask() const { return mask_; }
  uint64_t size() const { return mask_ + 1; }

private:
  uint32_t bits_{};
  uint64_t mask_{};
};

} //ns - poplar

#endif //POPLAR_TRIE_BASICS_HPP
