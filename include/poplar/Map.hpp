#ifndef POPLAR_TRIE_MAP_HPP
#define POPLAR_TRIE_MAP_HPP

#include <array>

#include "bit_tools.hpp"
#include "Exception.hpp"

namespace poplar {

// Associative array implementation with string keys based on a dynamic path-decomposed trie.
template <typename t_ht, typename t_ls, uint64_t t_lambda = 16>
class Map {
private:
  static_assert(is_power2(t_lambda));

public:
  using map_type = Map<t_ht, t_ls, t_lambda>; // Map Type
  using ht_type = t_ht; // HashTrie Type
  using ls_type = t_ls; // LabelStore Type
  using value_type = typename t_ls::value_type; // Value type

public:
  // Generic constructor.
  Map() = default;

  // Class constructor. Initially allocates the hash table of length 2**capa_bits.
  explicit Map(uint32_t capa_bits) {
    is_ready_ = true;
    hash_trie_ = ht_type{capa_bits, 8 + bit_tools::get_num_bits(t_lambda - 1)};
    label_store_ = ls_type{hash_trie_.capa_bits()};
    codes_.fill(UINT8_MAX);
    codes_[0] = static_cast<uint8_t>(num_codes_++); // terminator
  }

  // Generic destructor.
  ~Map() = default;

  // Searches the given key and returns the value pointer if registered; otherwise returns nullptr.
  const value_type* find(const char* key) const {
    return find_(make_ustr_view(key));
  }
  const value_type* find(const char* key, uint64_t len) const {
    return find_(make_ustr_view(key, len));
  }
  const value_type* find(const std::string& key) const {
    return find_(make_ustr_view(key));
  }
  const value_type* find(std::string_view key) const {
    return find_(make_ustr_view(key));
  }
  const value_type* find(ustr_view key) const {
    return find_(std::move(key));
  }

  // Inserts the given key and returns the value pointer.
  value_type* update(const char* key) {
    return update_(make_ustr_view(key));
  }
  value_type* update(const char* key, uint64_t len) {
    return update_(make_ustr_view(key, len));
  }
  value_type* update(const std::string& key) {
    return update_(make_ustr_view(key));
  }
  value_type* update(std::string_view key) {
    return update_(make_ustr_view(key));
  }
  value_type* update(ustr_view key) {
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
  void show_stat(std::ostream& os) const {
    os << "Statistics of Map\n";
    os << " - lambda: " << t_lambda << "\n";
    os << " - size: " << size() << "\n";
    os << " - capa_size: " << capa_size() << "\n";
    POPLAR_EX_STATS(
      os << " - num_steps: " << num_steps_ << "\n";
      os << " - num_resize: " << num_resize_ << "\n";
    )
    hash_trie_.show_stat(os);
    label_store_.show_stat(os);
  }

  // Swaps
  void swap(Map& rhs) {
    std::swap(is_ready_, rhs.is_ready_);
    std::swap(hash_trie_, rhs.hash_trie_);
    std::swap(label_store_, rhs.label_store_);
    std::swap(codes_, rhs.codes_);
    std::swap(num_codes_, rhs.num_codes_);
    std::swap(size_, rhs.size_);

    POPLAR_EX_STATS(
      std::swap(num_steps_, rhs.num_steps_);
      std::swap(num_resize_, rhs.num_resize_);
    )
  }

  Map(const Map&) = delete;
  Map& operator=(const Map&) = delete;

  Map(Map&& rhs) noexcept : Map() {
    this->swap(rhs);
  }
  Map& operator=(Map&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  static constexpr uint64_t NIL_ID = ht_type::NIL_ID;
  static constexpr uint64_t STEP_SYMB = UINT8_MAX; // (UINT8_MAX, 0)

  bool is_ready_{false};
  ht_type hash_trie_{};
  ls_type label_store_{};
  std::array<uint8_t, 256> codes_{};
  uint32_t num_codes_{};
  uint64_t size_{};

  POPLAR_EX_STATS(
    uint64_t num_steps_{};
    uint64_t num_resize_{};
  )

  const value_type* find_(ustr_view&& key) const {
    POPLAR_THROW_IF(key.empty(), "");
    POPLAR_THROW_IF(key.back() != '\0', "");

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

      while (t_lambda <= match) {
        node_id = hash_trie_.find_child(node_id, STEP_SYMB);
        if (node_id == NIL_ID) {
          return nullptr;
        }
        match -= t_lambda;
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

  value_type* update_(ustr_view&& key) {
    if (key.empty() || key.back() != '\0') {
      POPLAR_THROW("The query key is an invalid format.");
    }

    if (hash_trie_.size() == 0) {
      if (!is_ready_) {
        *this = map_type{0};
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

      key.remove_prefix(match);

      while (t_lambda <= match) {
        if (hash_trie_.add_child(node_id, STEP_SYMB)) {
          expand_if_needed_(node_id);
          POPLAR_EX_STATS(
            ++num_steps_;
          )
        }
        match -= t_lambda;
      }

      if (codes_[key[0]] == UINT8_MAX) {
        // Update table
        codes_[key[0]] = static_cast<uint8_t>(num_codes_++);
        POPLAR_THROW_IF(UINT8_MAX == num_codes_, "");
      }

      if (hash_trie_.add_child(node_id, make_symb_(key[0], match))) {
        expand_if_needed_(node_id);
        key.remove_prefix(1);
        ++size_;
        return label_store_.associate(node_id, key);
      }

      key.remove_prefix(1);
    }

    auto [vptr, match] = label_store_.compare(node_id, key);
    if (vptr != nullptr) {
      return const_cast<value_type*>(vptr);
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
      POPLAR_EX_STATS(
        ++num_resize_;
      )
    }
  }
};

} //ns - poplar

#endif //POPLAR_TRIE_MAP_HPP
