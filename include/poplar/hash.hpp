#ifndef POPLAR_TRIE_HASH_HPP
#define POPLAR_TRIE_HASH_HPP

#include "basics.hpp"

namespace poplar::hash {

// http://xoroshiro.di.unimi.it/splitmix64.c
struct vigna_hasher {
  vigna_hasher() = default;

  explicit vigna_hasher(uint64_t seed) : seed_(seed) {}

  static uint64_t hash(uint64_t x) {
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
  }
  uint64_t operator()(uint64_t x) const {
    x += seed_;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
  }

 private:
  uint64_t seed_ = 0x9e3779b97f4a7c15ULL;
};

}  // namespace poplar::hash

#endif  // POPLAR_TRIE_HASH_HPP
