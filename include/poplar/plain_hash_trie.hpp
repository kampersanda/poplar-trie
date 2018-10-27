#ifndef POPLAR_TRIE_PLAIN_HASH_TRIE_HPP
#define POPLAR_TRIE_PLAIN_HASH_TRIE_HPP

#include <iostream>

#include "bit_tools.hpp"
#include "bit_vector.hpp"
#include "compact_vector.hpp"
#include "hash.hpp"

namespace poplar {

// The node IDs are arranged incrementally
template <uint32_t MaxFactor = 80, typename Hasher = hash::vigna_hasher>
class plain_hash_trie {
  static_assert(0 < MaxFactor and MaxFactor < 100);

 public:
  using this_type = plain_hash_trie<MaxFactor, Hasher>;

  static constexpr uint64_t nil_id = UINT64_MAX;
  static constexpr uint32_t min_capa_bits = 16;

  static constexpr auto trie_type_id = trie_type_ids::HASH_TRIE;

 public:
  plain_hash_trie() = default;

  plain_hash_trie(uint32_t capa_bits, uint32_t symb_bits) {
    capa_size_ = size_p2{std::max(min_capa_bits, capa_bits)};
    symb_size_ = size_p2{symb_bits};
    max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);
    table_ = compact_vector{capa_size_.size(), capa_size_.bits() + symb_size_.bits()};
    ids_ = compact_vector{capa_size_.size(), capa_size_.bits()};
  }

  ~plain_hash_trie() = default;

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
    assert(node_id < capa_size_.size());
    assert(symb < symb_size_.size());

    if (size_ == 0) {
      return nil_id;
    }

    uint64_t key = make_key_(node_id, symb);

    for (uint64_t i = init_id_(key);; i = right_(i)) {
      uint64_t child_id = ids_[i];

      if (child_id == 0) {  // empty?
        return nil_id;
      }
      if (table_[i] == key) {
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

    uint64_t key = make_key_(node_id, symb);
    assert(key != 0);

    for (uint64_t i = init_id_(key);; i = right_(i)) {
      uint64_t child_id = ids_[i];

      if (child_id == 0) {  // empty?
        node_id = size_++;  // new child_id
        assert(node_id != 0);

        table_.set(i, key);
        ids_.set(i, node_id);

        return true;
      }

      if (table_[i] == key) {
        node_id = child_id;
        return false;  // already stored
      }
    }
  }

  // # of registerd nodes
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

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "plain_hash_trie");
    show_stat(os, indent, "factor", double(size()) / capa_size() * 100);
    show_stat(os, indent, "max_factor", MaxFactor);
    show_stat(os, indent, "size", size());
    show_stat(os, indent, "capa_bits", capa_bits());
    show_stat(os, indent, "symb_bits", symb_bits());
#ifdef POPLAR_EXTRA_STATS
    show_stat(os, indent, "num_resize", num_resize_);
#endif
  }

  plain_hash_trie(const plain_hash_trie&) = delete;
  plain_hash_trie& operator=(const plain_hash_trie&) = delete;

  plain_hash_trie(plain_hash_trie&&) noexcept = default;
  plain_hash_trie& operator=(plain_hash_trie&&) noexcept = default;

 private:
  compact_vector table_;
  compact_vector ids_;
  uint64_t size_ = 0;  // # of registered nodes
  uint64_t max_size_ = 0;  // MaxFactor% of the capacity
  size_p2 capa_size_;
  size_p2 symb_size_;
#ifdef POPLAR_EXTRA_STATS
  uint64_t num_resize_ = 0;
#endif

  uint64_t make_key_(uint64_t node_id, uint64_t symb) const {
    return (node_id << symb_size_.bits()) | symb;
  }
  uint64_t init_id_(uint64_t key) const {
    return Hasher::hash(key) & capa_size_.mask();
  }
  uint64_t right_(uint64_t slot_id) const {
    return (slot_id + 1) & capa_size_.mask();
  }

  void expand_() {
    this_type new_ht{capa_bits() + 1, symb_bits()};
#ifdef POPLAR_EXTRA_STATS
    new_ht.num_resize_ = num_resize_ + 1;
#endif

    for (uint64_t i = 0; i < capa_size_.size(); ++i) {
      uint64_t child_id = ids_[i];
      if (child_id == 0) {  // empty?
        continue;
      }

      uint64_t key = table_[i];
      assert(key != 0);

      for (uint64_t new_i = new_ht.init_id_(key);; new_i = new_ht.right_(new_i)) {
        if (new_ht.ids_[new_i] == 0) {  // empty?
          new_ht.table_.set(new_i, key);
          new_ht.ids_.set(new_i, child_id);
          break;
        }
      }
    }

    new_ht.size_ = size_;
    *this = std::move(new_ht);
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_HASH_TRIE_HPP
