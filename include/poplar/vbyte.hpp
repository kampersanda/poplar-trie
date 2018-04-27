#ifndef POPLAR_TRIE_VBYTE_HPP
#define POPLAR_TRIE_VBYTE_HPP

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
