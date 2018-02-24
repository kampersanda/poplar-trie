#ifndef POPLAR_TRIE_BIT_TOOLS_HPP
#define POPLAR_TRIE_BIT_TOOLS_HPP

#ifdef POPLAR_USE_POPCNT
#include <popcntintrin.h>
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
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

// Popcount
inline uint64_t popcnt(uint8_t x) {
  return POPCNT_TABLE[x];
}
inline uint64_t popcnt(uint16_t x) {
  return POPCNT_TABLE[x & UINT8_MAX] + POPCNT_TABLE[x >> 8];
}
inline uint64_t popcnt(uint32_t x) {
#ifdef POPLAR_USE_POPCNT
  return static_cast<uint64_t>(_mm_popcnt_u32(x));
#else
  x = (x & 0x55555555U) + ((x & 0xAAAAAAAAU) >> 1);
  x = (x & 0x33333333U) + ((x & 0xCCCCCCCCU) >> 2);
  x = (x & 0x0F0F0F0FU) + ((x & 0xF0F0F0F0U) >> 4);
  x *= 0x01010101U;
  return x >> 24;
#endif
}
inline uint64_t popcnt(uint64_t x) {
#ifdef POPLAR_USE_POPCNT
  return static_cast<uint64_t>(_mm_popcnt_u64(x));
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

constexpr uint32_t NUM_BIT_TABLE[256] = {
  1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

inline uint32_t get_num_bits(uint64_t x) {
  uint64_t x1 = x >> 32;
  if (x1 > 0) { // 32 < |x| <= 64
    uint64_t x2 = x1 >> 16;
    if (x2 > 0) { // 48 < |x| <= 64
      x1 = x2 >> 8;
      if (x1 > 0) { // 56 < |x| <= 64
        return NUM_BIT_TABLE[x1] + 56;
      } else { // 48 < |x| <= 56
        return NUM_BIT_TABLE[x2] + 48;
      }
    } else { // 32 < |x| <= 48
      x2 = x1 >> 8;
      if (x2 > 0) { // 40 < |x| <= 48
        return NUM_BIT_TABLE[x2] + 40;
      } else { // 32 < |x| <= 40
        return NUM_BIT_TABLE[x1] + 32;
      }
    }
  } else { // 0 < |x| <= 32
    uint64_t x2 = x >> 16;
    if (x2 > 0) { // 16 < |x| <= 32
      x1 = x2 >> 8;
      if (x1 > 0) { // 24 < |x| <= 32
        return NUM_BIT_TABLE[x1] + 24;
      } else { // 16 < |x| <= 24
        return NUM_BIT_TABLE[x2] + 16;
      }
    } else { // 0 < |x| <= 16
      x1 = x >> 8;
      if (x1 > 0) { // 8 < |x| <= 16
        return NUM_BIT_TABLE[x1] + 8;
      } else { // 0 < |x| <= 8
        return NUM_BIT_TABLE[x];
      }
    }
  }
}

} //ns - poplar::bit_tools

#endif //POPLAR_TRIE_BIT_TOOLS_HPP
