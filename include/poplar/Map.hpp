#ifndef POPLAR_TRIE_MAP_HPP
#define POPLAR_TRIE_MAP_HPP

#include <array>

#include "Exception.hpp"
#include "bit_tools.hpp"

namespace poplar {

// Associative array implementation with string keys based on a dynamic
// path-decomposed trie.
template <typename HashTrie, typename LabelStore, uint64_t Lambda = 16>
class Map {
 private:
  static_assert(is_power2(Lambda));

 public:
  using MapType = Map<HashTrie, LabelStore, Lambda>;  // Map Type
  using ValueType = typename LabelStore::ValueType;  // Value type

 public:
  // Generic constructor.
  Map() = default;

  // Class constructor. Initially allocates the hash table of length
  // 2**capa_bits.
  explicit Map(uint32_t capa_bits) {
    is_ready_ = true;
    hash_trie_ = HashTrie{capa_bits, 8 + bit_tools::get_num_bits(Lambda - 1)};
    label_store_ = LabelStore{hash_trie_.capa_bits()};
    codes_.fill(UINT8_MAX);
    codes_[0] = static_cast<uint8_t>(num_codes_++);  // terminator
  }

  // Generic destructor.
  ~Map() = default;

  // Searches the given key and returns the value pointer if registered;
  // otherwise returns nullptr.
  const ValueType* find(const char* key) const {
    return find_(make_ustr_view(key));
  }
  const ValueType* find(const char* key, uint64_t len) const {
    return find_(make_ustr_view(key, len));
  }
  const ValueType* find(const std::string& key) const {
    return find_(make_ustr_view(key));
  }
  const ValueType* find(std::string_view key) const {
    return find_(make_ustr_view(key));
  }
  const ValueType* find(ustr_view key) const {
    return find_(std::move(key));
  }

  // Inserts the given key and returns the value pointer.
  ValueType* update(const char* key) {
    return update_(make_ustr_view(key));
  }
  ValueType* update(const char* key, uint64_t len) {
    return update_(make_ustr_view(key, len));
  }
  ValueType* update(const std::string& key) {
    return update_(make_ustr_view(key));
  }
  ValueType* update(std::string_view key) {
    return update_(make_ustr_view(key));
  }
  ValueType* update(ustr_view key) {
    return update_(std::move(key));
  }

  // Gets the number of registered keys.
  uint64_t size() const {
    return size_;
  }

  // Gets the capacity of the hash table.
  uint64_t capa_size() const {
    return hash_trie_.capa_size();
  }

  // Shows the statistics.
  void show_stat(std::ostream& os, int level = 0) const {
    std::string indent(level, '\t');
    os << indent << "stat:Map\n";
    os << indent << "\tlambda:" << Lambda << "\n";
    os << indent << "\tsize:" << size() << "\n";
    os << indent << "\tcapa_size:" << capa_size() << "\n";
#ifdef POPLAR_ENABLE_EX_STATS
    os << indent << "\trate_steps:" << double(num_steps_) / hash_trie_.size() << "\n";
    os << indent << "\tnum_resize:" << num_resize_ << "\n";
#endif
    hash_trie_.show_stat(os, level + 1);
    label_store_.show_stat(os, level + 1);
  }

  Map(const Map&) = delete;
  Map& operator=(const Map&) = delete;

  Map(Map&&) noexcept = default;
  Map& operator=(Map&&) noexcept = default;

 private:
  static constexpr uint64_t NIL_ID = HashTrie::NIL_ID;
  static constexpr uint64_t STEP_SYMB = UINT8_MAX;  // (UINT8_MAX, 0)

  bool is_ready_{false};
  HashTrie hash_trie_{};
  LabelStore label_store_{};
  std::array<uint8_t, 256> codes_{};
  uint32_t num_codes_{};
  uint64_t size_{};
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t num_steps_{};
  uint64_t num_resize_{};
#endif

  const ValueType* find_(ustr_view&& key) const {
    POPLAR_THROW_IF(key.empty(), "key must be a non-empty string.");
    POPLAR_THROW_IF(key.back() != '\0', "The last character of key must be the null terminator.");

    if (!is_ready_ || hash_trie_.size() == 0) {
      return nullptr;
    }

    auto node_id = hash_trie_.get_root();

    while (!key.empty()) {
      auto [vptr, match] = label_store_.compare(node_id, key);
      if (vptr != nullptr) {
        return vptr;
      }

      key.remove_prefix(match);

      while (Lambda <= match) {
        node_id = hash_trie_.find_child(node_id, STEP_SYMB);
        if (node_id == NIL_ID) {
          return nullptr;
        }
        match -= Lambda;
      }

      if (codes_[key[0]] == UINT8_MAX) {
        // Detecting an useless character
        return nullptr;
      }

      node_id = hash_trie_.find_child(node_id, make_symb_(key[0], match));
      if (node_id == NIL_ID) {
        return nullptr;
      }

      key.remove_prefix(1);
    }

    return label_store_.compare(node_id, key).first;
  }

  ValueType* update_(ustr_view&& key) {
    POPLAR_THROW_IF(key.empty(), "key must be a non-empty string.");
    POPLAR_THROW_IF(key.back() != '\0', "The last character of key must be the null terminator.");

    if (hash_trie_.size() == 0) {
      if (!is_ready_) {
        *this = MapType{0};
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
        return const_cast<ValueType*>(vptr);
      }

      key.remove_prefix(match);

      while (Lambda <= match) {
        if (hash_trie_.add_child(node_id, STEP_SYMB) != ac_res_type::ALREADY_STORED) {
          expand_if_needed_(node_id);
#ifdef POPLAR_ENABLE_EX_STATS
          ++num_steps_;
#endif
        }
        match -= Lambda;
      }

      if (codes_[key[0]] == UINT8_MAX) {
        // Update table
        codes_[key[0]] = static_cast<uint8_t>(num_codes_++);
        POPLAR_THROW_IF(UINT8_MAX == num_codes_, "");
      }

      if (hash_trie_.add_child(node_id, make_symb_(key[0], match)) != ac_res_type::ALREADY_STORED) {
        expand_if_needed_(node_id);
        key.remove_prefix(1);
        ++size_;
        return label_store_.associate(node_id, key);
      }

      key.remove_prefix(1);
    }

    auto [vptr, match] = label_store_.compare(node_id, key);
    if (vptr != nullptr) {
      return const_cast<ValueType*>(vptr);
    }

    key.remove_prefix(match);
    ++size_;

    return label_store_.associate(node_id, key);
  }

  uint64_t make_symb_(uint8_t c, uint64_t match) const {
    assert(codes_[c] != UINT8_MAX);
    return static_cast<uint64_t>(codes_[c]) | (match << 8);
  }

  void expand_if_needed_(uint64_t& node_id) {
    if (hash_trie_.needs_to_expand()) {
      auto node_map = hash_trie_.expand();
      node_id = node_map[node_id];
      label_store_.expand(node_map);
#ifdef POPLAR_ENABLE_EX_STATS
      ++num_resize_;
#endif
    }
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_MAP_HPP
