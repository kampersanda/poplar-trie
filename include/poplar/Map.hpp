#ifndef POPLAR_TRIE_MAP_HPP
#define POPLAR_TRIE_MAP_HPP

#include <array>
#include <iostream>

#include "bit_tools.hpp"
#include "exception.hpp"

namespace poplar {

// Associative array implementation with string keys based on a dynamic
// path-decomposed trie.
template <typename HashTrie, typename LabelStore, uint64_t Lambda = 16>
class map {
  static_assert(is_power2(Lambda));
  static_assert(HashTrie::random_order == LabelStore::random_order);

 public:
  using this_type = map<HashTrie, LabelStore, Lambda>;  // map Type
  using value_type = typename LabelStore::value_type;  // Value type

  static constexpr bool random_order = HashTrie::random_order;

 public:
  // Generic constructor.
  map() = default;

  // Class constructor. Initially allocates the hash table of length
  // 2**capa_bits.
  explicit map(uint32_t capa_bits) {
    is_ready_ = true;
    hash_trie_ = HashTrie{capa_bits, 8 + bit_tools::get_num_bits(Lambda - 1)};
    label_store_ = LabelStore{hash_trie_.capa_bits()};
    codes_.fill(UINT8_MAX);
    codes_[0] = static_cast<uint8_t>(num_codes_++);  // terminator
  }

  // Generic destructor.
  ~map() = default;

  // Searches the given key and returns the value pointer if registered;
  // otherwise returns nullptr.
  const value_type* find(char_range key) const {
    POPLAR_THROW_IF(key.empty(), "key must be a non-empty string.");
    POPLAR_THROW_IF(*(key.end - 1) != '\0',
                    "The last character of key must be the null terminator.");

    if (!is_ready_ or hash_trie_.size() == 0) {
      return nullptr;
    }

    auto node_id = hash_trie_.get_root();

    while (!key.empty()) {
      auto [vptr, match] = label_store_.compare(node_id, key);
      if (vptr != nullptr) {
        return vptr;
      }

      key.begin += match;

      while (Lambda <= match) {
        node_id = hash_trie_.find_child(node_id, STEP_SYMB);
        if (node_id == nil_id) {
          return nullptr;
        }
        match -= Lambda;
      }

      if (codes_[*key.begin] == UINT8_MAX) {
        // Detecting an useless character
        return nullptr;
      }

      node_id = hash_trie_.find_child(node_id, make_symb_(*key.begin, match));
      if (node_id == nil_id) {
        return nullptr;
      }

      ++key.begin;
    }

    return label_store_.compare(node_id, key).first;
  }

  // Inserts the given key and returns the value pointer.
  value_type* update(char_range key) {
    POPLAR_THROW_IF(key.empty(), "key must be a non-empty string.");
    POPLAR_THROW_IF(*(key.end - 1) != '\0',
                    "The last character of key must be the null terminator.");

    if (hash_trie_.size() == 0) {
      if (!is_ready_) {
        *this = this_type{0};
      }
      // The first insertion
      ++size_;
      hash_trie_.add_root();
      return label_store_.associate(hash_trie_.get_root(), key);
    }

    auto node_id = hash_trie_.get_root();

    while (!key.empty()) {
      auto [vptr, match] = label_store_.compare(node_id, key);
      if (vptr != nullptr) {
        return const_cast<value_type*>(vptr);
      }

      key.begin += match;

      while (Lambda <= match) {
        if (hash_trie_.add_child(node_id, STEP_SYMB)) {
          expand_if_needed_(node_id);
#ifdef POPLAR_ENABLE_EX_STATS
          ++num_steps_;
#endif
          if constexpr (!random_order) {
            label_store_.dummy_associate(node_id);
          }
        }
        match -= Lambda;
      }

      if (codes_[*key.begin] == UINT8_MAX) {
        // Update table
        codes_[*key.begin] = static_cast<uint8_t>(num_codes_++);
        POPLAR_THROW_IF(UINT8_MAX == num_codes_, "");
      }

      if (hash_trie_.add_child(node_id, make_symb_(*key.begin, match))) {
        expand_if_needed_(node_id);
        ++key.begin;
        ++size_;
        return label_store_.associate(node_id, key);
      }

      ++key.begin;
    }

    auto [vptr, match] = label_store_.compare(node_id, key);
    return vptr ? const_cast<value_type*>(vptr) : nullptr;
  }

  // Gets the number of registered keys.
  uint64_t size() const {
    return size_;
  }
  // Gets the capacity of the hash table.
  uint64_t capa_size() const {
    return hash_trie_.capa_size();
  }

  auto make_ptree() const {
    boost::property_tree::ptree pt;
    pt.put("name", "map");
    pt.put("lambda", Lambda);
    pt.put("size", size());
    pt.put("capa_size", capa_size());
#ifdef POPLAR_ENABLE_EX_STATS
    pt.put("rate_steps", double(num_steps_) / hash_trie_.size());
#endif
    pt.add_child("hash_trie", hash_trie_.make_ptree());
    pt.add_child("label_store", label_store_.make_ptree());
    return pt;
  }

  map(const map&) = delete;
  map& operator=(const map&) = delete;

  map(map&&) noexcept = default;
  map& operator=(map&&) noexcept = default;

 private:
  static constexpr uint64_t nil_id = HashTrie::nil_id;
  static constexpr uint64_t STEP_SYMB = UINT8_MAX;  // (UINT8_MAX, 0)

  bool is_ready_ = false;
  HashTrie hash_trie_;
  LabelStore label_store_;
  std::array<uint8_t, 256> codes_ = {};
  uint32_t num_codes_ = 0;
  uint64_t size_ = 0;
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t num_steps_ = 0;
#endif

  uint64_t make_symb_(uint8_t c, uint64_t match) const {
    assert(codes_[c] != UINT8_MAX);
    return static_cast<uint64_t>(codes_[c]) | (match << 8);
  }

  void expand_if_needed_(uint64_t& node_id) {
    if constexpr (random_order) {
      if (!hash_trie_.needs_to_expand()) {
        return;
      }
      auto node_map = hash_trie_.expand();
      node_id = node_map[node_id];
      label_store_.expand(node_map);
    }
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_MAP_HPP
