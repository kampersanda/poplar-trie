#ifndef POPLAR_TRIE_HASH_HPP
#define POPLAR_TRIE_HASH_HPP

#include "basics.hpp"

namespace poplar::hash {

// http://xoroshiro.di.unimi.it/splitmix64.c
struct SplitMix {
  static uint64_t hash(uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
  }
};

}  // namespace poplar::hash

#endif  // POPLAR_TRIE_HASH_HPP
