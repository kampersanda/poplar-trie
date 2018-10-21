#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/compact_bonsai_trie.hpp"
#include "poplar/compact_hash_trie.hpp"
#include "poplar/plain_bonsai_trie.hpp"
#include "poplar/plain_hash_trie.hpp"

#include "poplar/compact_label_store_bt.hpp"
#include "poplar/compact_label_store_ht.hpp"
#include "poplar/plain_label_store_bt.hpp"
#include "poplar/plain_label_store_ht.hpp"

#include "poplar/map.hpp"

namespace poplar {

template <typename Value, uint64_t Lambda = 16>
using map_pp = map<plain_hash_trie<>, plain_label_store_ht<Value>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_pp_ex = map<plain_bonsai_trie<>, plain_label_store_bt<Value>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_pc = map<plain_hash_trie<>, compact_label_store_ht<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_pc_ex = map<plain_bonsai_trie<>, compact_label_store_bt<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_cp = map<compact_hash_trie<>, plain_label_store_ht<Value>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_cp_ex = map<compact_bonsai_trie<>, plain_label_store_bt<Value>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_cc = map<compact_hash_trie<>, compact_label_store_ht<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_cc_ex = map<compact_bonsai_trie<>, compact_label_store_bt<Value, ChunkSize>, Lambda>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
