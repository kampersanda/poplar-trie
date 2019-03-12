#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/compact_bonsai_trie.hpp"
#include "poplar/compact_fkhash_trie.hpp"
#include "poplar/plain_bonsai_trie.hpp"
#include "poplar/plain_fkhash_trie.hpp"

#include "poplar/compact_bonsai_nlm.hpp"
#include "poplar/compact_fkhash_nlm.hpp"
#include "poplar/plain_bonsai_nlm.hpp"
#include "poplar/plain_fkhash_nlm.hpp"

#include "poplar/map.hpp"

namespace poplar {

template <typename Value>
using plain_bonsai_map = map<plain_bonsai_trie<>, plain_bonsai_nlm<Value>>;

template <typename Value, uint64_t ChunkSize = 16>
using compact_bonsai_map = map<compact_bonsai_trie<>, compact_bonsai_nlm<Value, ChunkSize>>;

template <typename Value>
using plain_fkhash_map = map<plain_fkhash_trie<>, plain_fkhash_nlm<Value>>;

template <typename Value, uint64_t ChunkSize = 16>
using compact_fkhash_map = map<compact_fkhash_trie<>, compact_fkhash_nlm<Value, ChunkSize>>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
