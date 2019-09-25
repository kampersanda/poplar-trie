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
#ifndef POPLAR_TRIE_COMPACT_FKHASH_NLM_HPP
#define POPLAR_TRIE_COMPACT_FKHASH_NLM_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "vbyte.hpp"

namespace poplar {

template <typename Value, uint64_t ChunkSize = 16>
class compact_fkhash_nlm {
  public:
    using this_type = compact_fkhash_nlm<Value, ChunkSize>;
    using value_type = Value;
    using chunk_type = typename chunk_type_traits<ChunkSize>::type;

    static constexpr auto trie_type_id = trie_type_ids::FKHASH_TRIE;

  public:
    compact_fkhash_nlm() = default;

    explicit compact_fkhash_nlm(uint32_t capa_bits) {
        chunk_ptrs_.reserve((1ULL << capa_bits) / ChunkSize);
        chunk_buf_.reserve(1ULL << 10);
    }

    ~compact_fkhash_nlm() = default;

    std::pair<const value_type*, uint64_t> compare(uint64_t pos, const char_range& key) const {
        assert(pos < size_);

        const uint8_t* char_ptr = nullptr;
        auto [chunk_id, pos_in_chunk] = decompose_value<ChunkSize>(pos);

        if (chunk_id < chunk_ptrs_.size()) {
            char_ptr = chunk_ptrs_[chunk_id].get();
        } else {
            assert(chunk_id == chunk_ptrs_.size());
            char_ptr = chunk_buf_.data();
        }

        uint64_t alloc = 0;
        for (uint64_t i = 0; i < pos_in_chunk; ++i) {
            char_ptr += vbyte::decode(char_ptr, alloc);
            char_ptr += alloc;
        }
        char_ptr += vbyte::decode(char_ptr, alloc);

        if (key.empty()) {
            return {reinterpret_cast<const value_type*>(char_ptr), 0};
        }

        assert(sizeof(value_type) <= alloc);

        uint64_t length = alloc - sizeof(value_type);
        for (uint64_t i = 0; i < length; ++i) {
            if (key[i] != char_ptr[i]) {
                return {nullptr, i};
            }
        }

        if (key[length] != '\0') {
            return {nullptr, length};
        }

        // +1 considers the terminator '\0'
        return {reinterpret_cast<const value_type*>(char_ptr + length), length + 1};
    };

    value_type* append(const char_range& key) {
        auto [chunk_id, pos_in_chunk] = decompose_value<ChunkSize>(size_++);
        if (chunk_id != 0 && pos_in_chunk == 0) {
            release_buf_();
        }

#ifdef POPLAR_EXTRA_STATS
        max_length_ = std::max<uint64_t>(max_length_, key.length());
        sum_length_ += key.length();
#endif

        uint64_t length = key.empty() ? 0 : key.length() - 1;
        vbyte::append(chunk_buf_, length + sizeof(value_type));
        std::copy(key.begin, key.begin + length, std::back_inserter(chunk_buf_));
        for (size_t i = 0; i < sizeof(value_type); ++i) {
            chunk_buf_.emplace_back('\0');
        }

        return reinterpret_cast<value_type*>(chunk_buf_.data() + chunk_buf_.size() - sizeof(value_type));
    }

    // Associate a dummy label
    void append_dummy() {
        auto [chunk_id, pos_in_chunk] = decompose_value<ChunkSize>(size_++);
        if (chunk_id != 0 && pos_in_chunk == 0) {
            release_buf_();
        }

        vbyte::append(chunk_buf_, 0);
    }

    uint64_t size() const {
        return size_;
    }
    uint64_t num_ptrs() const {
        return chunk_ptrs_.size();
    }
    uint64_t alloc_bytes() const {
        uint64_t bytes = 0;
        bytes += chunk_ptrs_.capacity() * sizeof(std::unique_ptr<uint8_t[]>);
        bytes += chunk_buf_.capacity();
        bytes += label_bytes_;
        return bytes;
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "compact_fkhash_nlm");
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "num_ptrs", num_ptrs());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "max_length", max_length_);
        show_stat(os, indent, "ave_length", double(sum_length_) / size());
#endif
        show_stat(os, indent, "chunk_size", ChunkSize);
    }

    compact_fkhash_nlm(const compact_fkhash_nlm&) = delete;
    compact_fkhash_nlm& operator=(const compact_fkhash_nlm&) = delete;

    compact_fkhash_nlm(compact_fkhash_nlm&&) noexcept = default;
    compact_fkhash_nlm& operator=(compact_fkhash_nlm&&) noexcept = default;

  private:
    std::vector<std::unique_ptr<uint8_t[]>> chunk_ptrs_;
    std::vector<uint8_t> chunk_buf_;  // for the last chunk
    uint64_t size_ = 0;
    uint64_t label_bytes_ = 0;

#ifdef POPLAR_EXTRA_STATS
    uint64_t max_length_ = 0;
    uint64_t sum_length_ = 0;
#endif

    void release_buf_() {
        label_bytes_ += chunk_buf_.size();
        auto new_uptr = std::make_unique<uint8_t[]>(chunk_buf_.size());
        std::copy(chunk_buf_.begin(), chunk_buf_.end(), new_uptr.get());
        chunk_ptrs_.emplace_back(std::move(new_uptr));
        chunk_buf_.clear();
    }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_FKHASH_NLM_HPP
