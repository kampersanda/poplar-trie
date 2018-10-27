#ifndef POPLAR_TRIE_HASH_TRIE_MAP_HPP
#define POPLAR_TRIE_HASH_TRIE_MAP_HPP

#include <poplar.hpp>

namespace poplar {

template <typename Trie, typename Value>
class hash_trie_map {
 public:
  using this_type = hash_trie_map<Trie, Value>;
  using trie_type = Trie;
  using value_type = Value;
  using cht_type = compact_hash_table<sizeof(Value) * 8>;

  static constexpr auto nil = static_cast<value_type>(-1);
  static constexpr auto trie_type_id = Trie::trie_type_id;

 public:
  hash_trie_map() = default;

  explicit hash_trie_map(uint32_t capa_bits) {
    trie_ = trie_type(capa_bits, 8);
    trie_.add_root();
    cht_ = cht_type(trie_.capa_bits());
    is_ready_ = true;
  }

  ~hash_trie_map() = default;

  value_type find(const std::string& key) const {
    if (!is_ready_) {
      return nil;
    }

    auto node_id = trie_.get_root();
    for (uint8_t c : key) {
      node_id = trie_.find_child(node_id, c);
      if (node_id == Trie::nil_id) {
        return nil;
      }
    }
    auto v = cht_.get(node_id);
    if (v == cht_type::nil) {
      return nil;
    }
    return static_cast<value_type>(v);
  }

  bool update(const std::string& key, value_type val) {
    if (!is_ready_) {
      trie_ = Trie(0, 8);
      trie_.add_root();
      cht_ = cht_type(trie_.capa_bits());
      is_ready_ = true;
    }

    auto node_id = trie_.get_root();

    for (uint8_t c : key) {
      if (trie_.add_child(node_id, c)) {
        if constexpr (trie_type_id == trie_type_ids::BONSAI_TRIE) {
          if (!trie_.needs_to_expand()) {
            continue;
          }
          auto node_map = trie_.expand();
          node_id = node_map[node_id];
          cht_type new_cht(trie_.capa_bits());
          for (uint64_t i = 0; i < node_map.size(); ++i) {  // TODO: speed-up
            if (node_map[i] != UINT64_MAX) {
              uint64_t v = cht_.get(i);
              if (v != cht_type::nil) {
                new_cht.set(node_map[i], v);
              }
            }
          }
          cht_ = std::move(new_cht);
        } else {  // trie_type_id == trie_type_ids::HASH_TRIE
          if (cht_.univ_bits() < trie_.capa_bits()) {
            cht_type new_cht(trie_.capa_bits());
            for (uint64_t i = 0; i < cht_.univ_size(); ++i) {  // TODO: speed-up
              uint64_t v = cht_.get(i);
              if (v != cht_type::nil) {
                if (!new_cht.set(i, v)) {
                  assert(false);
                }
              }
            }
            cht_ = std::move(new_cht);
          }
        }
      }
    }

    return cht_.set(node_id, val);
  }

  uint64_t size() const {
    return cht_.size();
  }
  uint64_t num_nodes() const {
    return trie_.size();
  }

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "hash_trie_map");
    show_stat(os, indent, "size", size());
    show_stat(os, indent, "num_nodes", num_nodes());
    show_member(os, indent, "trie_");
    trie_.show_stats(os, n + 1);
    show_member(os, indent, "cht_");
    cht_.show_stats(os, n + 1);
  }

  hash_trie_map(const hash_trie_map&) = delete;
  hash_trie_map& operator=(const hash_trie_map&) = delete;

  hash_trie_map(hash_trie_map&&) noexcept = default;
  hash_trie_map& operator=(hash_trie_map&&) noexcept = default;

 private:
  bool is_ready_ = false;
  trie_type trie_;
  cht_type cht_;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_HASH_TRIE_MAP_HPP