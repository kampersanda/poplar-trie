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
#ifndef POPLAR_TRIE_PLAIN_BONSAI_NLM_HPP
#define POPLAR_TRIE_PLAIN_BONSAI_NLM_HPP

#include <memory>
#include <vector>

#include "basics.hpp"
#include "compact_vector.hpp"

namespace poplar {

template <typename Value>
class plain_bonsai_nlm {
  public:
    using value_type = Value;

    static constexpr auto trie_type_id = trie_type_ids::BONSAI_TRIE;

  public:
    plain_bonsai_nlm() = default;

    explicit plain_bonsai_nlm(uint32_t capa_bits) : ptrs_(1ULL << capa_bits) {}

    ~plain_bonsai_nlm() = default;

    std::pair<const value_type*, uint64_t> compare(uint64_t pos, const char_range& key) const {
        assert(pos < ptrs_.size());
        assert(ptrs_[pos]);

        const uint8_t* ptr = ptrs_[pos].get();

        if (key.empty()) {
            return {reinterpret_cast<const value_type*>(ptr), 0};
        }

        for (uint64_t i = 0; i < key.length(); ++i) {
            if (key[i] != ptr[i]) {
                return {nullptr, i};
            }
        }

        return {reinterpret_cast<const value_type*>(ptr + key.length()), key.length()};
    }

    value_type* insert(uint64_t pos, const char_range& key) {
        assert(!ptrs_[pos]);

        ++size_;

        uint64_t length = key.length();
        ptrs_[pos] = std::make_unique<uint8_t[]>(length + sizeof(value_type));
        auto ptr = ptrs_[pos].get();
        copy_bytes(ptr, key.begin, length);

        label_bytes_ += length + sizeof(value_type);

#ifdef POPLAR_EXTRA_STATS
        max_length_ = std::max(max_length_, length);
        sum_length_ += length;
#endif

        auto ret = reinterpret_cast<value_type*>(ptr + length);
        *ret = static_cast<value_type>(0);

        return ret;
    }

    template <typename T>
    void expand(const T& pos_map) {
        std::vector<std::unique_ptr<uint8_t[]>> new_ptrs(ptrs_.size() * 2);
        for (uint64_t i = 0; i < pos_map.size(); ++i) {
            if (pos_map[i] != UINT64_MAX) {
                new_ptrs[pos_map[i]] = std::move(ptrs_[i]);
            }
        }
        ptrs_ = std::move(new_ptrs);
    }

    uint64_t size() const {
        return size_;
    }
    uint64_t num_ptrs() const {
        return ptrs_.size();
    }
    uint64_t alloc_bytes() const {
        uint64_t bytes = 0;
        bytes += ptrs_.capacity() * sizeof(std::unique_ptr<uint8_t[]>);
        bytes += label_bytes_;
        return bytes;
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "plain_bonsai_nlm");
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "num_ptrs", num_ptrs());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "max_length", max_length_);
        show_stat(os, indent, "ave_length", double(sum_length_) / size());
#endif
    }

    plain_bonsai_nlm(const plain_bonsai_nlm&) = delete;
    plain_bonsai_nlm& operator=(const plain_bonsai_nlm&) = delete;

    plain_bonsai_nlm(plain_bonsai_nlm&&) noexcept = default;
    plain_bonsai_nlm& operator=(plain_bonsai_nlm&&) noexcept = default;

  private:
    std::vector<std::unique_ptr<uint8_t[]>> ptrs_;
    uint64_t size_ = 0;
    uint64_t label_bytes_ = 0;
#ifdef POPLAR_EXTRA_STATS
    uint64_t max_length_ = 0;
    uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_BONSAI_NLM_HPP
