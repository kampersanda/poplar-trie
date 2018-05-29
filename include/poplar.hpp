#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/CompactCuckooHashTrie.hpp"
#include "poplar/CompactHashTrie.hpp"
#include "poplar/PlainHashTrie.hpp"

#include "poplar/EmbeddedLabelStore.hpp"
#include "poplar/GroupedLabelStore.hpp"
#include "poplar/PlainLabelStore.hpp"

#include "poplar/Map.hpp"

namespace poplar {

template <typename t_value, uint64_t t_lambda = 16>
using MapPP = Map<PlainHashTrie<>, PlainLabelStore<t_value>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 64, uint64_t t_lambda = 16>
using MapPE = Map<PlainHashTrie<>, EmbeddedLabelStore<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 16, uint64_t t_lambda = 16>
using MapPG = Map<PlainHashTrie<>, GroupedLabelStore<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_lambda = 16>
using MapCP = Map<CompactHashTrie<>, PlainLabelStore<t_value>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 64, uint64_t t_lambda = 16>
using MapCE = Map<CompactHashTrie<>, EmbeddedLabelStore<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 16, uint64_t t_lambda = 16>
using MapCG = Map<CompactHashTrie<>, GroupedLabelStore<t_value, BitChunk<t_chunk_size>>, t_lambda>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
