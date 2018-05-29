#ifndef POPLAR_TRIE_COMPACT_HASH_TABLE_HPP
#define POPLAR_TRIE_COMPACT_HASH_TABLE_HPP

#include "Exception.hpp"
#include "IntVector.hpp"
#include "bijective_hash.hpp"
#include "bit_tools.hpp"

namespace poplar {

template <uint32_t ValBits, uint32_t Factor = 80, typename Hasher = bijective_hash::SplitMix>
class CompactHashTable {
 private:
  static_assert(0 < Factor and Factor < 100);

 public:
  using ThisType = CompactHashTable<ValBits, Factor, Hasher>;

  static constexpr uint32_t MIN_CAPA_BITS = 12;
  static constexpr uint32_t VAL_BITS = ValBits;
  static constexpr uint64_t VAL_MASK = (1ULL << ValBits) - 1;

 public:
  CompactHashTable() = default;

  explicit CompactHashTable(uint32_t univ_bits, uint32_t capa_bits = MIN_CAPA_BITS) {
    univ_size_ = size_p2_t{univ_bits};
    capa_size_ = size_p2_t{std::max(MIN_CAPA_BITS, capa_bits)};

    assert(capa_size_.bits() <= univ_size_.bits());

    quo_size_ = size_p2_t{univ_size_.bits() - capa_size_.bits()};
    quo_shift_ = 2 + VAL_BITS;
    quo_invmask_ = ~(quo_size_.mask() << quo_shift_);

    max_size_ = static_cast<uint64_t>(capa_size_.size() * Factor / 100.0);

    hash_ = Hasher{univ_size_.bits()};
    table_ = IntVector{capa_size_.size(), quo_size_.bits() + VAL_BITS + 2, (VAL_MASK << 2) | 1ULL};
  }

  ~CompactHashTable() = default;

  uint64_t get(uint64_t key) const {
    assert(key < univ_size_.size());

    const decomp_val_t dec = decompose_(hash_.hash(key));

    if (!get_vbit_(dec.mod)) {
      return UINT64_MAX;
    }

    uint64_t slot_id = find_ass_cbit_(dec.mod);
    if (slot_id == UINT64_MAX) {
      return UINT64_MAX;
    }

    if (!find_item_(slot_id, dec.quo)) {
      return UINT64_MAX;
    }
    return get_val_(slot_id);
  }

  bool set(uint64_t key, uint64_t val) {
    assert(key < univ_size_.size());
    assert(val < VAL_MASK);

    expand_if_needed_();

    const decomp_val_t dec = decompose_(hash_.hash(key));

    if (is_vacant_(dec.mod)) {
      // without collision
      update_slot_(dec.mod, dec.quo, val, true, true);
      ++size_;
      return true;
    }

    uint64_t empty_id = 0;
    uint64_t slot_id = find_ass_cbit_(dec.mod, empty_id);

    if (!get_vbit_(dec.mod)) {  // initial insertion in the group
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
      set_vbit_(dec.mod, true);
      set_cbit_(empty_id, true);
    } else {
      // collision group already exists
      if (find_item_(slot_id, dec.quo)) {  // already registered?
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

    set_quo_(empty_id, dec.quo);
    set_val_(empty_id, val);

    ++size_;

    return true;
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

  void show_stat(std::ostream& os, int level = 0) const {
    std::string indent(level, '\t');
    os << indent << "stat:CompactHashTable\n";
    os << indent << "\tfactor:" << Factor << "\n";
    os << indent << "\tsize:" << size() << "\n";
    os << indent << "\tuniv_size:" << univ_size() << "\n";
    os << indent << "\tuniv_bits:" << univ_bits() << "\n";
    os << indent << "\tcapa_size:" << capa_size() << "\n";
    os << indent << "\tcapa_bits:" << capa_bits() << "\n";
#ifdef POPLAR_ENABLE_EX_STATS
    os << indent << "\tnum_resize:" << num_resize_ << "\n";
#endif
    hash_.show_stat(os, level + 1);
  }

  CompactHashTable(const CompactHashTable&) = delete;
  CompactHashTable& operator=(const CompactHashTable&) = delete;

  CompactHashTable(CompactHashTable&&) noexcept = default;
  CompactHashTable& operator=(CompactHashTable&&) noexcept = default;

 private:
  Hasher hash_{};
  IntVector table_{};
  uint64_t size_{};  // # of registered nodes
  uint64_t max_size_{};  // Factor% of the capacity
  size_p2_t univ_size_{};
  size_p2_t capa_size_{};
  size_p2_t quo_size_{};
  uint64_t quo_shift_{};
  uint64_t quo_invmask_{};  // For setter

#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t num_resize_{};
#endif

  uint64_t find_ass_cbit_(uint64_t slot_id) const {
    uint64_t dummy{};
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

  void expand_if_needed_() {
    if (size_ < max_size_) {
      return;
    }

    ThisType new_cht{univ_size_.bits(), capa_size_.bits() + 1};

#ifdef POPLAR_ENABLE_EX_STATS
    new_cht.num_resize_ = num_resize_ + 1;
#endif

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

          uint64_t key = hash_.hash_inv((get_quo_(i) << capa_size_.bits()) | init_id);
          uint64_t val = get_val_(i);
          new_cht.set(key, val);

          i = right_(i);
          if (i == beg) {
            completed = true;
          }
        } while (!get_cbit_(i));

        init_id = right_(init_id);
      } while (i != init_id);
    }

    assert(size() == new_cht.size());
    *this = std::move(new_cht);
  }

  decomp_val_t decompose_(uint64_t x) const {
    return {x >> capa_size_.bits(), x & capa_size_.mask()};
  }

  uint64_t left_(uint64_t slot_id) const {
    return (slot_id - 1) & capa_size_.mask();
  }
  uint64_t right_(uint64_t slot_id) const {
    return (slot_id + 1) & capa_size_.mask();
  }
  bool is_vacant_(uint64_t slot_id) const {
    return get_val_(slot_id) == VAL_MASK;
  }

  uint64_t get_quo_(uint64_t slot_id) const {
    return table_.get(slot_id) >> quo_shift_;
  }
  uint64_t get_val_(uint64_t slot_id) const {
    return (table_.get(slot_id) >> 2) & VAL_MASK;
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
    assert(val <= VAL_MASK);
    table_.set(slot_id, (table_.get(slot_id) & ~(VAL_MASK << 2)) | (val << 2));
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
    assert(val <= VAL_MASK);
    table_.set(slot_id, (quo << quo_shift_) | (val << 2) | (vbit << 1) | cbit);
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_HASH_TABLE_HPP
