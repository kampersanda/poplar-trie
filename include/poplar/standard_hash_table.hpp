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
#ifndef POPLAR_STANDARD_HASH_TABLE_HPP
#define POPLAR_STANDARD_HASH_TABLE_HPP

#include "exception.hpp"
#include "hash.hpp"

namespace poplar {

template <uint32_t MaxFactor = 80, typename Hasher = hash::vigna_hasher>
class standard_hash_table {
    static_assert(0 < MaxFactor and MaxFactor < 100);

  public:
    using this_type = standard_hash_table<MaxFactor, Hasher>;

    static constexpr uint32_t min_capa_bits = 6;
    static constexpr uint64_t nil = UINT64_MAX;

  public:
    standard_hash_table() = default;

    explicit standard_hash_table(uint32_t capa_bits) {
        capa_size_ = size_p2(std::max(min_capa_bits, capa_bits));
        max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);
        table_.resize(capa_size_.size());
    }

    ~standard_hash_table() = default;

    uint64_t get(uint64_t key) const {
        if (table_.empty()) {
            return nil;
        }

        for (uint64_t i = init_id_(key);; i = right_(i)) {
            if (table_[i].key == UINT64_MAX) {
                return nil;
            }
            if (table_[i].key == key) {
                return table_[i].val;
            }
        }
    }

    bool set(uint64_t key, uint64_t val) {
        if (table_.empty()) {
            *this = this_type(min_capa_bits);
        }

        if (max_size_ <= size_) {
            expand_();
        }

        for (uint64_t i = init_id_(key);; i = right_(i)) {
            if (table_[i].key == UINT64_MAX) {
                table_[i] = {key, val};
                ++size_;
                return true;
            }
            if (table_[i].key == key) {
                table_[i].val = val;
                return false;
            }
        }
    }

    uint64_t size() const {
        return size_;
    }
    uint64_t max_size() const {
        return max_size_;
    }
    uint64_t capa_size() const {
        return capa_size_.size();
    }
    uint32_t capa_bits() const {
        return capa_size_.bits();
    }
    uint64_t alloc_bytes() const {
        return table_.capacity() * sizeof(slot_type);
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "standard_hash_table");
        show_stat(os, indent, "factor", double(size()) / capa_size() * 100);
        show_stat(os, indent, "max_factor", MaxFactor);
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "capa_size", capa_size());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "num_resize", num_resize_);
#endif
    }

    standard_hash_table(const standard_hash_table&) = delete;
    standard_hash_table& operator=(const standard_hash_table&) = delete;

    standard_hash_table(standard_hash_table&&) noexcept = default;
    standard_hash_table& operator=(standard_hash_table&&) noexcept = default;

  private:
    struct slot_type {
        uint64_t key = UINT64_MAX;
        uint64_t val = 0;
    };

    std::vector<slot_type> table_;
    uint64_t size_ = 0;  // # of registered nodes
    uint64_t max_size_ = 0;  // MaxFactor% of the capacity
    size_p2 capa_size_;
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize_ = 0;
#endif

    uint64_t init_id_(uint64_t key) const {
        return Hasher::hash(key) & capa_size_.mask();
    }
    uint64_t right_(uint64_t slot_id) const {
        return (slot_id + 1) & capa_size_.mask();
    }

    void expand_() {
        this_type new_ht{capa_size_.bits() + 1};
#ifdef POPLAR_EXTRA_STATS
        new_ht.num_resize_ = num_resize_ + 1;
#endif

        for (uint64_t i = 0; i < table_.size(); ++i) {
            if (table_[i].key != UINT64_MAX) {
                new_ht.set(table_[i].key, table_[i].val);
            }
        }

        assert(size() == new_ht.size());
        *this = std::move(new_ht);
    }
};

}  // namespace poplar

#endif  // POPLAR_STANDARD_HASH_TABLE_HPP
