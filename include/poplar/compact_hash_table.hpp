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
#ifndef POPLAR_TRIE_COMPACT_HASH_TABLE_HPP
#define POPLAR_TRIE_COMPACT_HASH_TABLE_HPP

#include "bijective_hash.hpp"
#include "bit_tools.hpp"
#include "compact_vector.hpp"
#include "exception.hpp"

namespace poplar {

template <uint32_t ValBits, uint32_t MaxFactor = 80, typename Hasher = bijective_hash::split_mix_hasher>
class compact_hash_table {
    static_assert(0 < MaxFactor and MaxFactor < 100);

  public:
    using this_type = compact_hash_table<ValBits, MaxFactor, Hasher>;

    static constexpr uint32_t min_capa_bits = 12;
    static constexpr uint32_t val_bits = ValBits;
    static constexpr uint64_t val_mask = (1ULL << ValBits) - 1;
    static constexpr uint64_t nil = UINT64_MAX;

  public:
    compact_hash_table() = default;

    explicit compact_hash_table(uint32_t univ_bits, uint32_t capa_bits = min_capa_bits) {
        univ_size_ = size_p2{univ_bits};
        capa_size_ = size_p2{std::max(min_capa_bits, capa_bits)};

        assert(capa_size_.bits() <= univ_size_.bits());

        quo_size_ = size_p2{univ_size_.bits() - capa_size_.bits()};
        quo_shift_ = 2 + val_bits;
        quo_invmask_ = ~(quo_size_.mask() << quo_shift_);

        max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);

        hasher_ = Hasher{univ_size_.bits()};
        table_ = compact_vector{capa_size_.size(), quo_size_.bits() + val_bits + 2, (val_mask << 2) | 1ULL};
    }

    ~compact_hash_table() = default;

    uint64_t get(uint64_t key) const {
        assert(key < univ_size_.size());

        auto [quo, mod] = decompose_(hasher_.hash(key));

        if (!get_vbit_(mod)) {
            return nil;
        }

        uint64_t slot_id = find_ass_cbit_(mod);
        if (slot_id == UINT64_MAX) {
            return nil;
        }

        if (!find_item_(slot_id, quo)) {
            return nil;
        }
        return get_val_(slot_id);
    }

    bool set(uint64_t key, uint64_t val) {
        assert(key < univ_size_.size());
        assert(val < val_mask);

        if (max_size_ <= size_) {
            // expand
            this_type new_cht{univ_size_.bits(), capa_size_.bits() + 1};
#ifdef POPLAR_EXTRA_STATS
            new_cht.num_resize_ = num_resize_ + 1;
#endif
            clone(new_cht);
            *this = std::move(new_cht);
        }

        auto [quo, mod] = decompose_(hasher_.hash(key));

        if (is_vacant_(mod)) {
            // without collision
            update_slot_(mod, quo, val, true, true);
            ++size_;
            return true;
        }

        uint64_t empty_id = 0;
        uint64_t slot_id = find_ass_cbit_(mod, empty_id);

        if (!get_vbit_(mod)) {  // initial insertion in the group
            // create a new collision group
            if (slot_id != UINT64_MAX) {  // require to displace existing groups?
                do {
                    slot_id = right_(slot_id);
                } while (!get_cbit_(slot_id));

                slot_id = left_(slot_id);  // rightmost slot of the group

                while (empty_id != slot_id) {
                    empty_id = copy_from_right_(empty_id);
                }
            } else {
                // not inside other collision groups
            }
            set_vbit_(mod, true);
            set_cbit_(empty_id, true);
        } else {
            // collision group already exists
            if (find_item_(slot_id, quo)) {  // already registered?
                set_val_(slot_id, val);  // update
                return false;
            }

            slot_id = left_(slot_id);  // rightmost of the group

            // displace existing groups for creating an empty slot
            while (empty_id != slot_id) {
                empty_id = copy_from_right_(empty_id);
            }
            set_cbit_(empty_id, false);
        }

        set_quo_(empty_id, quo);
        set_val_(empty_id, val);

        ++size_;

        return true;
    }

    void clone(this_type& new_cht) const {
        set_mapper mapper;
        clone(new_cht, mapper);
    }
    template <class SetMapper>
    void clone(this_type& new_cht, const SetMapper& mapper) const {
        POPLAR_THROW_IF(new_cht.size() != 0, "new_cht must be empty.");
        POPLAR_THROW_IF(new_cht.max_size() < size(), "this->size() <= new_cht.max_size() must hold.");

        // Find the first vacant slot
        uint64_t i = 0;
        while (!is_vacant_(i)) {
            i = right_(i);
        }

        const uint64_t beg = i;
        i = right_(i);  // skip the vacant

        for (bool completed = false; !completed;) {
            // Find the leftmost of some collision groups
            while (is_vacant_(i)) {
                i = right_(i);
                if (i == beg) {
                    completed = true;
                }
            }

            assert(get_cbit_(i));
            uint64_t init_id = i;

            do {
                // Find the rightmost of the collision group
                while (!get_vbit_(init_id)) {
                    init_id = right_(init_id);
                }

                do {
                    assert(!is_vacant_(i));

                    uint64_t key = hasher_.hash_inv((get_quo_(i) << capa_size_.bits()) | init_id);
                    uint64_t val = get_val_(i);
                    // new_cht.set(key, val);
                    mapper(new_cht, key, val);

                    i = right_(i);
                    if (i == beg) {
                        completed = true;
                    }
                } while (!get_cbit_(i));

                init_id = right_(init_id);
            } while (i != init_id);
        }

        assert(size() == new_cht.size());
    }

    uint64_t size() const {
        return size_;
    }
    uint64_t max_size() const {
        return max_size_;
    }
    uint64_t univ_size() const {
        return univ_size_.size();
    }
    uint32_t univ_bits() const {
        return univ_size_.bits();
    }
    uint64_t capa_size() const {
        return capa_size_.size();
    }
    uint32_t capa_bits() const {
        return capa_size_.bits();
    }
    uint64_t alloc_bytes() const {
        return table_.alloc_bytes();
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "compact_hash_table");
        show_stat(os, indent, "factor", double(size()) / capa_size() * 100);
        show_stat(os, indent, "max_factor", MaxFactor);
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "capa_size", capa_size());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "num_resize", num_resize_);
#endif
        show_member(os, indent, "hasher_");
        hasher_.show_stats(os, n + 1);
    }

    compact_hash_table(const compact_hash_table&) = delete;
    compact_hash_table& operator=(const compact_hash_table&) = delete;

    compact_hash_table(compact_hash_table&&) noexcept = default;
    compact_hash_table& operator=(compact_hash_table&&) noexcept = default;

  private:
    Hasher hasher_;
    compact_vector table_;
    uint64_t size_ = 0;  // # of registered nodes
    uint64_t max_size_ = 0;  // MaxFactor% of the capacity
    size_p2 univ_size_;
    size_p2 capa_size_;
    size_p2 quo_size_;
    uint64_t quo_shift_ = 0;
    uint64_t quo_invmask_ = 0;  // For setter
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize_ = 0;
#endif

    struct set_mapper {
        void operator()(this_type& new_cht, uint64_t key, uint64_t val) const {
            new_cht.set(key, val);
        }
    };

    uint64_t find_ass_cbit_(uint64_t slot_id) const {
        uint64_t dummy = 0;
        return find_ass_cbit_(slot_id, dummy);
    }

    uint64_t find_ass_cbit_(uint64_t slot_id, uint64_t& empty_id) const {
        uint64_t num_vbits = 0;
        do {
            if (get_vbit_(slot_id)) {
                ++num_vbits;
            }
            slot_id = left_(slot_id);
        } while (!is_vacant_(slot_id));

        empty_id = slot_id;

        if (num_vbits == 0) {
            return UINT64_MAX;
        }

        uint64_t num_cbits = 0;
        while (num_cbits < num_vbits) {
            slot_id = right_(slot_id);
            if (get_cbit_(slot_id)) {
                ++num_cbits;
            }
        }

        return slot_id;
    }

    bool find_item_(uint64_t& slot_id, uint64_t quo) const {
        do {
            if (get_quo_(slot_id) == quo) {
                return true;
            }
            slot_id = right_(slot_id);
        } while (!get_cbit_(slot_id));
        return false;
    }

    std::pair<uint64_t, uint64_t> decompose_(uint64_t x) const {
        return {x >> capa_size_.bits(), x & capa_size_.mask()};
    }

    uint64_t left_(uint64_t slot_id) const {
        return (slot_id - 1) & capa_size_.mask();
    }
    uint64_t right_(uint64_t slot_id) const {
        return (slot_id + 1) & capa_size_.mask();
    }
    bool is_vacant_(uint64_t slot_id) const {
        return get_val_(slot_id) == val_mask;
    }

    uint64_t get_quo_(uint64_t slot_id) const {
        return table_.get(slot_id) >> quo_shift_;
    }
    uint64_t get_val_(uint64_t slot_id) const {
        return (table_.get(slot_id) >> 2) & val_mask;
    }
    bool get_vbit_(uint64_t slot_id) const {
        return (table_.get(slot_id) & 2) == 2;
    }
    bool get_cbit_(uint64_t slot_id) const {
        return (table_.get(slot_id) & 1) == 1;
    }

    void set_quo_(uint64_t slot_id, uint64_t quo) {
        assert(quo < quo_size_.size());
        table_.set(slot_id, (table_.get(slot_id) & quo_invmask_) | (quo << quo_shift_));
    }
    void set_val_(uint64_t slot_id, uint64_t val) {
        assert(val <= val_mask);
        table_.set(slot_id, (table_.get(slot_id) & ~(val_mask << 2)) | (val << 2));
    }
    void set_vbit_(uint64_t slot_id, bool bit) {
        table_.set(slot_id, (table_.get(slot_id) & ~2ULL) | (bit << 1));
    }
    void set_cbit_(uint64_t slot_id, bool bit) {
        table_.set(slot_id, (table_.get(slot_id) & ~1ULL) | bit);
    }

    // Copies the slot from the right one except virgin bit information.
    uint64_t copy_from_right_(uint64_t slot_id) {
        uint64_t _slot_id = right_(slot_id);
        table_.set(slot_id, (table_.get(_slot_id) & ~2ULL) | (get_vbit_(slot_id) << 1));
        return _slot_id;
    }

    void update_slot_(uint64_t slot_id, uint64_t quo, uint64_t val, bool vbit, bool cbit) {
        assert(quo < quo_size_.size());
        assert(val <= val_mask);
        table_.set(slot_id, (quo << quo_shift_) | (val << 2) | (vbit << 1) | cbit);
    }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_HASH_TABLE_HPP
