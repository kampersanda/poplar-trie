#ifndef POPLAR_TRIE_POPLAR_HPP
#define POPLAR_TRIE_POPLAR_HPP

#include "poplar/HashTrieCR.hpp"
#include "poplar/HashTriePR.hpp"
#include "poplar/LabelStoreEM.hpp"
#include "poplar/LabelStoreGM.hpp"
#include "poplar/LabelStorePM.hpp"
#include "poplar/Map.hpp"

namespace poplar {

template <typename t_value, uint64_t t_lambda = 16>
using MapPP = Map<HashTriePR<>, LabelStorePM<t_value>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 64, uint64_t t_lambda = 16>
using MapPE =
    Map<HashTriePR<>, LabelStoreEM<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 16, uint64_t t_lambda = 16>
using MapPG =
    Map<HashTriePR<>, LabelStoreGM<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_lambda = 16>
using MapCP = Map<HashTrieCR<>, LabelStorePM<t_value>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 64, uint64_t t_lambda = 16>
using MapCE =
    Map<HashTrieCR<>, LabelStoreEM<t_value, BitChunk<t_chunk_size>>, t_lambda>;

template <typename t_value, uint64_t t_chunk_size = 16, uint64_t t_lambda = 16>
using MapCG =
    Map<HashTrieCR<>, LabelStoreGM<t_value, BitChunk<t_chunk_size>>, t_lambda>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
