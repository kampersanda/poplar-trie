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
#ifndef POPLAR_TRIE_COMPACT_BONSAI_TRIE_HPP
#define POPLAR_TRIE_COMPACT_BONSAI_TRIE_HPP

#include "bijective_hash.hpp"
#include "bit_vector.hpp"
#include "compact_hash_table.hpp"
#include "compact_vector.hpp"
#include "standard_hash_table.hpp"

namespace poplar {

template <uint32_t MaxFactor = 90, uint32_t Dsp1Bits = 4, class AuxCht = compact_hash_table<7>,
          class AuxMap = standard_hash_table<>, class Hasher = bijective_hash::split_mix_hasher>
class compact_bonsai_trie {
    static_assert(0 < MaxFactor and MaxFactor < 100);
    static_assert(0 < Dsp1Bits and Dsp1Bits < 64);

  public:
    using this_type = compact_bonsai_trie<MaxFactor, Dsp1Bits, AuxCht, AuxMap, Hasher>;
    using aux_cht_type = AuxCht;
    using aux_map_type = AuxMap;

    static constexpr uint64_t nil_id = UINT64_MAX;
    static constexpr uint32_t min_capa_bits = 16;

    static constexpr uint32_t dsp1_bits = Dsp1Bits;
    static constexpr uint64_t dsp1_mask = (1ULL << dsp1_bits) - 1;
    static constexpr uint32_t dsp2_bits = aux_cht_type::val_bits;
    static constexpr uint32_t dsp2_mask = aux_cht_type::val_mask;

    static constexpr auto trie_type_id = trie_type_ids::BONSAI_TRIE;

  public:
    compact_bonsai_trie() = default;

    compact_bonsai_trie(uint32_t capa_bits, uint32_t symb_bits, uint32_t cht_capa_bits = 0) {
        capa_size_ = size_p2{std::max(min_capa_bits, capa_bits)};
        symb_size_ = size_p2{symb_bits};

        max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);

        hasher_ = Hasher{capa_size_.bits() + symb_size_.bits()};
        table_ = compact_vector{capa_size_.size(), symb_size_.bits() + dsp1_bits};
        aux_cht_ = aux_cht_type{capa_size_.bits(), cht_capa_bits};
    }

    ~compact_bonsai_trie() = default;

    uint64_t get_root() const {
        assert(size_ != 0);
        return 0;
    }

    void add_root() {
        assert(size_ == 0);
        size_ = 1;
    }

    uint64_t find_child(uint64_t node_id, uint64_t symb) const {
        assert(node_id < capa_size_.size());
        assert(symb < symb_size_.size());

        if (size_ == 0) {
            return nil_id;
        }

        auto [quo, mod] = decompose_(hasher_.hash(make_key_(node_id, symb)));

        for (uint64_t i = mod, cnt = 1;; i = right_(i), ++cnt) {
            if (i == get_root()) {
                // because the root's dsp value is zero though it is defined
                continue;
            }

            if (compare_dsp_(i, 0)) {
                // this slot is empty
                return nil_id;
            }

            if (compare_dsp_(i, cnt) and quo == get_quo_(i)) {
                return i;
            }
        }
    }

    bool add_child(uint64_t& node_id, uint64_t symb) {
        assert(node_id < capa_size_.size());
        assert(symb < symb_size_.size());

        auto [quo, mod] = decompose_(hasher_.hash(make_key_(node_id, symb)));

        for (uint64_t i = mod, cnt = 1;; i = right_(i), ++cnt) {
            // because the root's dsp value is zero though it is defined
            if (i == get_root()) {
                continue;
            }

            if (compare_dsp_(i, 0)) {
                // this slot is empty
                if (size_ == max_size_) {
                    return false;  // needs to expand
                }

                update_slot_(i, quo, cnt);

                ++size_;
                node_id = i;

                return true;
            }

            if (compare_dsp_(i, cnt) and quo == get_quo_(i)) {
                node_id = i;
                return false;  // already stored
            }
        }
    }

    std::pair<uint64_t, uint64_t> get_parent_and_symb(uint64_t node_id) const {
        assert(node_id < capa_size_.size());

        if (compare_dsp_(node_id, 0)) {
            // root or not exist
            return {nil_id, 0};
        }

        uint64_t dist = get_dsp_(node_id) - 1;
        uint64_t init_id = dist <= node_id ? node_id - dist : table_.size() - (dist - node_id);
        uint64_t key = hasher_.hash_inv(get_quo_(node_id) << capa_size_.bits() | init_id);

        // Returns pair (parent, label)
        return std::make_pair(key >> symb_size_.bits(), key & symb_size_.mask());
    }

    class node_map {
      public:
        node_map() = default;

        node_map(compact_vector&& map_high, compact_vector&& map_low, bit_vector&& done_flags)
            : map_high_(std::move(map_high)), map_low_(std::move(map_low)), done_flags_(std::move(done_flags)) {}

        ~node_map() = default;

        uint64_t operator[](uint64_t i) const {
            if (!done_flags_[i]) {
                return UINT64_MAX;
            }
            if (map_high_.size() == 0) {
                return map_low_[i];
            } else {
                return map_low_[i] | (map_high_[i] << map_low_.width());
            }
        }

        uint64_t size() const {
            return map_low_.size();
        }

        node_map(const node_map&) = delete;
        node_map& operator=(const node_map&) = delete;

        node_map(node_map&&) noexcept = default;
        node_map& operator=(node_map&&) noexcept = default;

      private:
        compact_vector map_high_;
        compact_vector map_low_;
        bit_vector done_flags_;
    };

    bool needs_to_expand() const {
        return max_size() <= size();
    }

    node_map expand() {
        // this_type new_ht{capa_bits() + 1, symb_size_.bits(), aux_cht_.capa_bits()};
        this_type new_ht{capa_bits() + 1, symb_size_.bits()};
        new_ht.add_root();

#ifdef POPLAR_EXTRA_STATS
        new_ht.num_resize_ = num_resize_ + 1;
#endif

        bit_vector done_flags(capa_size());
        done_flags.set(get_root());

        size_p2 low_size{table_.width()};
        compact_vector map_high;

        if (low_size.bits() < new_ht.capa_bits()) {
            map_high = compact_vector(capa_size(), new_ht.capa_bits() - low_size.bits());
        }

        std::vector<std::pair<uint64_t, uint64_t>> path;
        path.reserve(256);

        auto get_mapping = [&](uint64_t i) -> uint64_t {
            if (map_high.size() == 0) {
                return table_[i];
            } else {
                return table_[i] | (map_high[i] << low_size.bits());
            }
        };

        auto set_mapping = [&](uint64_t i, uint64_t v) {
            if (map_high.size() == 0) {
                table_.set(i, v);
            } else {
                table_.set(i, v & low_size.mask());
                map_high.set(i, v >> low_size.bits());
            }
        };

        // 0 is root
        for (uint64_t i = 1; i < table_.size(); ++i) {
            if (done_flags[i] or compare_dsp_(i, 0)) {
                // skip already processed or empty elements
                continue;
            }

            path.clear();
            uint64_t node_id = i;

            do {
                auto [parent, label] = get_parent_and_symb(node_id);
                assert(parent != nil_id);
                path.emplace_back(std::make_pair(node_id, label));
                node_id = parent;
            } while (!done_flags[node_id]);

            uint64_t new_node_id = get_mapping(node_id);

            for (auto rit = std::rbegin(path); rit != std::rend(path); ++rit) {
                new_ht.add_child(new_node_id, rit->second);
                set_mapping(rit->first, new_node_id);
                done_flags.set(rit->first);
            }
        }

        node_map node_map{std::move(map_high), std::move(table_), std::move(done_flags)};
        std::swap(*this, new_ht);

        return node_map;
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
        return bytes;
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "compact_hash_trie");
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

    compact_bonsai_trie(const compact_bonsai_trie&) = delete;
    compact_bonsai_trie& operator=(const compact_bonsai_trie&) = delete;

    compact_bonsai_trie(compact_bonsai_trie&&) noexcept = default;
    compact_bonsai_trie& operator=(compact_bonsai_trie&&) noexcept = default;

  private:
    Hasher hasher_;
    compact_vector table_;
    aux_cht_type aux_cht_;  // 2nd dsp
    aux_map_type aux_map_;  // 3rd dsp
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

    void update_slot_(uint64_t slot_id, uint64_t quo, uint64_t dsp) {
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
    }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_BONSAI_TRIE_HPP
