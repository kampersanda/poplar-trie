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
#ifndef POPLAR_TRIE_COMPACT_VECTOR_HPP
#define POPLAR_TRIE_COMPACT_VECTOR_HPP

#include <vector>

#include "bit_tools.hpp"
#include "exception.hpp"

namespace poplar {

class compact_vector {
  public:
    compact_vector() = default;

    compact_vector(uint64_t size, uint32_t width) {
        POPLAR_THROW_IF(64 <= width, "width overflow.");

        size_ = size;
        mask_ = (1ULL << width) - 1;
        width_ = width;
        chunks_.resize(bit_tools::words_for(size_ * width_), 0);
    }

    compact_vector(uint64_t size, uint32_t width, uint64_t init) : compact_vector{size, width} {
        for (uint64_t i = 0; i < size; ++i) {
            set(i, init);
        }
    }

    ~compact_vector() = default;

    void resize(uint64_t size) {
        size_ = size;
        chunks_.resize(bit_tools::words_for(size_ * width_));
    }

    uint64_t operator[](uint64_t i) const {
        return get(i);
    }

    uint64_t get(uint64_t i) const {
        assert(i < size_);

        auto [quo, mod] = decompose_value<64>(i * width_);

        if (mod + width_ <= 64) {
            return (chunks_[quo] >> mod) & mask_;
        } else {
            return ((chunks_[quo] >> mod) | (chunks_[quo + 1] << (64 - mod))) & mask_;
        }
    }

    void set(uint64_t i, uint64_t v) {
        assert(i < size_);
        assert(v <= mask_);

        auto [quo, mod] = decompose_value<64>(i * width_);

        chunks_[quo] &= ~(mask_ << mod);
        chunks_[quo] |= (v & mask_) << mod;

        if (64 < mod + width_) {
            const uint64_t diff = 64 - mod;
            chunks_[quo + 1] &= ~(mask_ >> diff);
            chunks_[quo + 1] |= (v & mask_) >> diff;
        }
    }

    uint64_t size() const {
        return size_;
    }
    uint32_t width() const {
        return width_;
    }
    uint64_t alloc_bytes() const {
        return chunks_.capacity() * sizeof(uint64_t);
    }

    compact_vector(const compact_vector&) = delete;
    compact_vector& operator=(const compact_vector&) = delete;

    compact_vector(compact_vector&&) noexcept = default;
    compact_vector& operator=(compact_vector&&) noexcept = default;

  private:
    std::vector<uint64_t> chunks_;
    uint64_t size_ = 0;
    uint64_t mask_ = 0;
    uint64_t width_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_VECTOR_HPP
