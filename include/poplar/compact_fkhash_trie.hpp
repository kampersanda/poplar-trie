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
#ifndef POPLAR_TRIE_COMPACT_FKHASH_TRIE_HPP
#define POPLAR_TRIE_COMPACT_FKHASH_TRIE_HPP

#include "bijective_hash.hpp"
#include "bit_vector.hpp"
#include "compact_hash_table.hpp"
#include "compact_vector.hpp"
#include "standard_hash_table.hpp"

namespace poplar {

template <uint32_t MaxFactor = 90, uint32_t Dsp1Bits = 4, class AuxCht = compact_hash_table<7>,
          class AuxMap = standard_hash_table<>, class Hasher = bijective_hash::split_mix_hasher>
class compact_fkhash_trie {
    static_assert(0 < MaxFactor and MaxFactor < 100);
    static_assert(0 < Dsp1Bits and Dsp1Bits < 64);

  public:
    using this_type = compact_fkhash_trie<MaxFactor, Dsp1Bits, AuxCht, AuxMap, Hasher>;
    using aux_cht_type = AuxCht;
    using aux_map_type = AuxMap;

    static constexpr uint64_t nil_id = UINT64_MAX;
    static constexpr uint32_t min_capa_bits = 16;

    static constexpr uint32_t dsp1_bits = Dsp1Bits;
    static constexpr uint64_t dsp1_mask = (1ULL << dsp1_bits) - 1;
    static constexpr uint32_t dsp2_bits = aux_cht_type::val_bits;
    static constexpr uint32_t dsp2_mask = aux_cht_type::val_mask;

    static constexpr auto trie_type_id = trie_type_ids::FKHASH_TRIE;

  public:
    compact_fkhash_trie() = default;

    compact_fkhash_trie(uint32_t capa_bits, uint32_t symb_bits, uint32_t cht_capa_bits = 0) {
        capa_size_ = size_p2{std::max(min_capa_bits, capa_bits)};
        symb_size_ = size_p2{symb_bits};
        max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);
        hasher_ = Hasher{capa_size_.bits() + symb_size_.bits()};
        table_ = compact_vector{capa_size_.size(), symb_size_.bits() + dsp1_bits};
        aux_cht_ = aux_cht_type{capa_size_.bits(), cht_capa_bits};
        ids_ = compact_vector{capa_size_.size(), capa_size_.bits(), capa_size_.mask()};
    }

    ~compact_fkhash_trie() = default;

    // The root ID is assigned but its slot does not exist in the table
    uint64_t get_root() const {
        assert(size_ != 0);
        return 0;
    }

    void add_root() {
        assert(size_ == 0);
        size_ = 1;
    }

    uint64_t find_child(uint64_t node_id, uint64_t symb) const {
        if (size_ == 0) {
            return nil_id;
        }

        auto [quo, mod] = decompose_(hasher_.hash(make_key_(node_id, symb)));

        for (uint64_t i = mod, cnt = 0;; i = right_(i), ++cnt) {
            uint64_t child_id = ids_[i];

            if (child_id == capa_size_.mask()) {
                // encounter an empty slot
                return nil_id;
            }

            if (compare_dsp_(i, cnt) and quo == get_quo_(i)) {
                return child_id;
            }
        }
    }

    bool add_child(uint64_t& node_id, uint64_t symb) {
        assert(node_id < capa_size_.size());
        assert(symb < symb_size_.size());

        if (max_size() <= size()) {
            expand_();
        }

        auto [quo, mod] = decompose_(hasher_.hash(make_key_(node_id, symb)));

        for (uint64_t i = mod, cnt = 0;; i = right_(i), ++cnt) {
            uint64_t child_id = ids_[i];

            if (child_id == capa_size_.mask()) {
                // encounter an empty slot
                update_slot_(i, quo, cnt, size_);
                node_id = size_++;
                return true;
            }

            if (compare_dsp_(i, cnt) and quo == get_quo_(i)) {
                node_id = child_id;
                return false;  // already stored
            }
        }
    }

    bool needs_to_expand() const {
        return max_size() <= size();
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
    uint64_t symb_size() const {
        return symb_size_.size();
    }
    uint32_t symb_bits() const {
        return symb_size_.bits();
    }
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize() const {
        return num_resize_;
    }
#endif
    uint64_t alloc_bytes() const {
        uint64_t bytes = 0;
        bytes += table_.alloc_bytes();
        bytes += aux_cht_.alloc_bytes();
        bytes += aux_map_.alloc_bytes();
        bytes += ids_.alloc_bytes();
        return bytes;
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "compact_fkhash_trie");
        show_stat(os, indent, "factor", double(size()) / capa_size() * 100);
        show_stat(os, indent, "max_factor", MaxFactor);
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
        show_stat(os, indent, "capa_bits", capa_bits());
        show_stat(os, indent, "symb_bits", symb_bits());
        show_stat(os, indent, "dsp1st_bits", dsp1_bits);
        show_stat(os, indent, "dsp2nd_bits", dsp2_bits);
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "rate_dsp1st", double(num_dsps_[0]) / size());
        show_stat(os, indent, "rate_dsp2nd", double(num_dsps_[1]) / size());
        show_stat(os, indent, "rate_dsp3rd", double(num_dsps_[2]) / size());
        show_stat(os, indent, "num_resize", num_resize_);
#endif
        show_member(os, indent, "hasher_");
        hasher_.show_stats(os, n + 1);
        show_member(os, indent, "aux_cht_");
        aux_cht_.show_stats(os, n + 1);
        show_member(os, indent, "aux_map_");
        aux_map_.show_stats(os, n + 1);
    }

    compact_fkhash_trie(const compact_fkhash_trie&) = delete;
    compact_fkhash_trie& operator=(const compact_fkhash_trie&) = delete;

    compact_fkhash_trie(compact_fkhash_trie&&) noexcept = default;
    compact_fkhash_trie& operator=(compact_fkhash_trie&&) noexcept = default;

  private:
    Hasher hasher_;
    compact_vector table_;
    aux_cht_type aux_cht_;  // 2nd dsp
    aux_map_type aux_map_;  // 3rd dsp
    compact_vector ids_;
    uint64_t size_ = 0;  // # of registered nodes
    uint64_t max_size_ = 0;  // MaxFactor% of the capacity
    size_p2 capa_size_;
    size_p2 symb_size_;
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize_ = 0;
    uint64_t num_dsps_[3] = {};
#endif

    uint64_t make_key_(uint64_t node_id, uint64_t symb) const {
        return (node_id << symb_size_.bits()) | symb;
    }
    std::pair<uint64_t, uint64_t> decompose_(uint64_t x) const {
        return {x >> capa_size_.bits(), x & capa_size_.mask()};
    }
    uint64_t right_(uint64_t slot_id) const {
        return (slot_id + 1) & capa_size_.mask();
    }

    uint64_t get_quo_(uint64_t slot_id) const {
        return table_[slot_id] >> dsp1_bits;
    }
    uint64_t get_dsp_(uint64_t slot_id) const {
        uint64_t dsp = table_[slot_id] & dsp1_mask;
        if (dsp < dsp1_mask) {
            return dsp;
        }

        dsp = aux_cht_.get(slot_id);
        if (dsp != aux_cht_type::nil) {
            return dsp + dsp1_mask;
        }

        return aux_map_.get(slot_id);
    }

    bool compare_dsp_(uint64_t slot_id, uint64_t rhs) const {
        uint64_t lhs = table_[slot_id] & dsp1_mask;
        if (lhs < dsp1_mask) {
            return lhs == rhs;
        }
        if (rhs < dsp1_mask) {
            return false;
        }

        lhs = aux_cht_.get(slot_id);
        if (lhs != aux_cht_type::nil) {
            return lhs + dsp1_mask == rhs;
        }
        if (rhs < dsp1_mask + dsp2_mask) {
            return false;
        }

        auto val = aux_map_.get(slot_id);
        assert(val != aux_map_type::nil);
        return val == rhs;
    }

    void update_slot_(uint64_t slot_id, uint64_t quo, uint64_t dsp, uint64_t node_id) {
        assert(table_[slot_id] == 0);
        assert(quo < symb_size_.size());

        uint64_t v = quo << dsp1_bits;

        if (dsp < dsp1_mask) {
            v |= dsp;
        } else {
            v |= dsp1_mask;
            uint64_t _dsp = dsp - dsp1_mask;
            if (_dsp < dsp2_mask) {
                aux_cht_.set(slot_id, _dsp);
            } else {
                aux_map_.set(slot_id, dsp);
            }
        }

#ifdef POPLAR_EXTRA_STATS
        if (dsp < dsp1_mask) {
            ++num_dsps_[0];
        } else if (dsp < dsp1_mask + dsp2_mask) {
            ++num_dsps_[1];
        } else {
            ++num_dsps_[2];
        }
#endif

        table_.set(slot_id, v);
        ids_.set(slot_id, node_id);
    }

    void expand_() {
        this_type new_ht{capa_bits() + 1, symb_size_.bits()};
#ifdef POPLAR_EXTRA_STATS
        new_ht.num_resize_ = num_resize_ + 1;
#endif

        for (uint64_t i = 0; i < capa_size_.size(); ++i) {
            uint64_t node_id = ids_[i];

            if (node_id == capa_size_.mask()) {
                // encounter an empty slot
                continue;
            }

            uint64_t dist = get_dsp_(i);
            uint64_t init_id = dist <= i ? i - dist : table_.size() - (dist - i);
            uint64_t key = hasher_.hash_inv(get_quo_(i) << capa_size_.bits() | init_id);

            auto [quo, mod] = new_ht.decompose_(new_ht.hasher_.hash(key));

            for (uint64_t new_i = mod, cnt = 0;; new_i = new_ht.right_(new_i), ++cnt) {
                if (new_ht.ids_[new_i] == new_ht.capa_size_.mask()) {
                    // encounter an empty slot
                    new_ht.update_slot_(new_i, quo, cnt, node_id);
                    break;
                }
            }
        }

        new_ht.size_ = size_;
        std::swap(*this, new_ht);
    }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_FKHASH_TRIE_HPP
