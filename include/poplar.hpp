#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/compact_hash_trie.hpp"
#include "poplar/compact_hash_trie_r.hpp"
#include "poplar/plain_hash_trie.hpp"
#include "poplar/plain_hash_trie_r.hpp"

#include "poplar/compact_label_store.hpp"
#include "poplar/compact_label_store_r.hpp"
#include "poplar/plain_label_store.hpp"
#include "poplar/plain_label_store_r.hpp"

#include "poplar/map.hpp"

namespace poplar {

template <typename Value, uint64_t Lambda = 16>
using map_pp = map<plain_hash_trie<>, plain_label_store<Value>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_pp_r = map<plain_hash_trie_r<>, plain_label_store_r<Value>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_pc = map<plain_hash_trie<>, compact_label_store<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_pc_r = map<plain_hash_trie_r<>, compact_label_store_r<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_cp = map<compact_hash_trie<>, plain_label_store<Value>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using map_cp_r = map<compact_hash_trie_r<>, plain_label_store_r<Value>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_cc = map<compact_hash_trie<>, compact_label_store<Value, ChunkSize>, Lambda>;

template <typename Value, uint64_t ChunkSize = 16, uint64_t Lambda = 16>
using map_cc_r = map<compact_hash_trie_r<>, compact_label_store_r<Value, ChunkSize>, Lambda>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
