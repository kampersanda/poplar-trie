/**
 * MIT License
 *
 * Copyright (c) 2018–2019 Shunsuke Kanda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
using semi_compact_bonsai_map = map<plain_bonsai_trie<>, compact_bonsai_nlm<Value, ChunkSize>>;

template <typename Value, uint64_t ChunkSize = 16>
using compact_bonsai_map = map<compact_bonsai_trie<>, compact_bonsai_nlm<Value, ChunkSize>>;

template <typename Value>
using plain_fkhash_map = map<plain_fkhash_trie<>, plain_fkhash_nlm<Value>>;

template <typename Value, uint64_t ChunkSize = 16>
using semi_compact_fkhash_map = map<plain_fkhash_trie<>, compact_fkhash_nlm<Value, ChunkSize>>;

template <typename Value, uint64_t ChunkSize = 16>
using compact_fkhash_map = map<compact_fkhash_trie<>, compact_fkhash_nlm<Value, ChunkSize>>;

}  // namespace poplar

#endif  // POPLAR_TRIE_POPLAR_HPP
