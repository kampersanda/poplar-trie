#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/CompactHashTrie.hpp"
#include "poplar/PlainHashTrie.hpp"

#include "poplar/EmbeddedLabelStore.hpp"
#include "poplar/GroupedLabelStore.hpp"
#include "poplar/PlainLabelStore.hpp"

#include "poplar/Map.hpp"

namespace poplar {

template <typename Value, uint64_t Lambda = 16>
using MapPP = Map<PlainHashTrie<>, PlainLabelStore<Value>, Lambda>;

template <typename Value, uint64_t t_chunk_size = 64, uint64_t Lambda = 16>
using MapPE = Map<PlainHashTrie<>, EmbeddedLabelStore<Value, BitChunk<t_chunk_size>>, Lambda>;

template <typename Value, uint64_t t_chunk_size = 16, uint64_t Lambda = 16>
using MapPG = Map<PlainHashTrie<>, GroupedLabelStore<Value, BitChunk<t_chunk_size>>, Lambda>;

template <typename Value, uint64_t Lambda = 16>
using MapCP = Map<CompactHashTrie<>, PlainLabelStore<Value>, Lambda>;

template <typename Value, uint64_t t_chunk_size = 64, uint64_t Lambda = 16>
using MapCE = Map<CompactHashTrie<>, EmbeddedLabelStore<Value, BitChunk<t_chunk_size>>, Lambda>;

template <typename Value, uint64_t t_chunk_size = 16, uint64_t Lambda = 16>
using MapCG = Map<CompactHashTrie<>, GroupedLabelStore<Value, BitChunk<t_chunk_size>>, Lambda>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
