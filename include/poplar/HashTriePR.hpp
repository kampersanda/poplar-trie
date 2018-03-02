#ifndef POPLAR_TRIE_HASH_TRIE_PR_HPP
#define POPLAR_TRIE_HASH_TRIE_PR_HPP

#include "bit_tools.hpp"
#include "BitVector.hpp"
#include "IntVector.hpp"
#include "hash.hpp"

namespace poplar {

template <uint32_t t_factor = 80, typename t_hash = hash::SplitMix>
class HashTriePR {
private:
  static_assert(0 < t_factor && t_factor < 100);

public:
  static constexpr uint64_t NIL_ID = UINT64_MAX;
  static constexpr uint32_t MIN_CAPA_BITS = 16;

public:
  HashTriePR() = default;

  HashTriePR(uint32_t capa_bits, uint32_t symb_bits) {
    capa_size_ = size_p2_t{std::max(MIN_CAPA_BITS, capa_bits)};
    symb_size_ = size_p2_t{symb_bits};

    max_size_ = static_cast<uint64_t>(capa_size_.size() * t_factor / 100.0);

    table_ = IntVector{capa_size_.size(), capa_size_.bits() + symb_size_.bits()};
  }

  ~HashTriePR() = default;

  uint64_t get_root() const {
    assert(size_ != 0);
    return 1;
  }

  void add_root() {
    assert(size_ == 0);
    size_ = 1;
  }

  uint64_t find_child(uint64_t node_id, uint64_t symb) const {
    assert(node_id < capa_size_.size());
    assert(symb < symb_size_.size());

    if (size_ == 0) {
      return NIL_ID;
    }

    uint64_t key = make_key_(node_id, symb);
    assert(key != 0);

    for (uint64_t i = t_hash::hash(key) & capa_size_.mask();; i = right_(i)) {
      if (i == 0) {
        // table_[0] is always empty so that table_[i] = 0 indicates to be empty.
        continue;
      }
      if (i == get_root()) {
        continue;
      }
      if (table_[i] == 0) {
        // encounter an empty slot
        return NIL_ID;
      }
      if (table_[i] == key) {
        return i;
      }
    }
  }

  int add_child(uint64_t& node_id, uint64_t symb) {
    assert(node_id < capa_size_.size());
    assert(symb < symb_size_.size());

    uint64_t key = make_key_(node_id, symb);
    assert(key != 0);

    for (uint64_t i = t_hash::hash(key) & capa_size_.mask();; i = right_(i)) {
      if (i == 0) {
        // table_[0] is always empty so that any table_[i] = 0 indicates to be empty.
        continue;
      }
      if (i == get_root()) {
        continue;
      }
      if (table_[i] == 0) {
        // this slot is empty
        if (size_ == max_size_) {
          return 2; // fail to register because of full
        }
        ++size_;
        table_.set(i, key);
        node_id = i;
        return 1; // success to register
      }
      if (table_[i] == key) {
        node_id = i;
        return 0;
      }
    }
  }

  std::pair<uint64_t, uint64_t> get_parent_and_symb(uint64_t node_id) const {
    assert(node_id < capa_size_.size());

    const uint64_t key = table_[node_id];

    if (key == 0) {
      // root or not exist
      return {NIL_ID, 0};
    }

    // Returns pair (parent, label)
    return std::make_pair(key >> symb_size_.bits(), key & symb_size_.mask());
  };

  class NodeMap {
  public:
    //!
    NodeMap() = default;

    NodeMap(IntVector&& map, BitVector&& done_flags)
      : map_{std::move(map)}, done_flags_{std::move(done_flags)} {}

    ~NodeMap() = default;

    uint64_t operator[](uint64_t i) const {
      return done_flags_[i] ? map_[i] : UINT64_MAX;
    }

    uint64_t size() const {
      return map_.size();
    }

    void swap(NodeMap& rhs) {
      std::swap(map_, rhs.map_);
      std::swap(done_flags_, rhs.done_flags_);
    }

    NodeMap(const NodeMap&) = delete;
    NodeMap& operator=(const NodeMap&) = delete;

    NodeMap(NodeMap&& rhs) noexcept : NodeMap() {
      this->swap(rhs);
    }
    NodeMap& operator=(NodeMap&& rhs) noexcept {
      this->swap(rhs);
      return *this;
    }

  private:
    IntVector map_{};
    BitVector done_flags_{};
  };

  bool needs_to_expand() const {
    return max_size() <= size();
  }

  NodeMap expand() {
    HashTriePR new_ht{capa_bits() + 1, symb_size_.bits()};
    new_ht.add_root();

    POPLAR_EX_STATS(
      new_ht.num_resize_ = num_resize_ + 1;
    )

    BitVector done_flags(capa_size());
    done_flags.set(get_root());

    table_.set(get_root(), new_ht.get_root());

    std::vector<std::pair<uint64_t, uint64_t>> path;
    path.reserve(256);

    // 0 is empty, 1 is root
    for (uint64_t i = 2; i < table_.size(); ++i) {
      if (done_flags[i] || table_[i] == 0) {
        // skip already processed or empty elements
        continue;
      }

      path.clear();
      uint64_t node_id = i;

      do {
        auto [parent, label] = get_parent_and_symb(node_id);
        assert(parent != NIL_ID);
        path.emplace_back(std::make_pair(node_id, label));
        node_id = parent;
      } while (!done_flags[node_id]);

      uint64_t new_node_id = table_[node_id];

      for (auto rit = std::rbegin(path); rit != std::rend(path); ++rit) {
        int ret = new_ht.add_child(new_node_id, rit->second);
        assert(ret == 1);
        table_.set(rit->first, new_node_id);
        done_flags.set(rit->first);
      }
    }

    NodeMap node_map{std::move(table_), std::move(done_flags)};
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

  void show_stat(std::ostream& os, std::string&& level = "") const {
    os << level << "stat:HashTriePR\n";
    os << level << "\tfactor:" << t_factor << "\n";
    os << level << "\tsize:" << size() << "\n";
    os << level << "\tcapa_size:" << capa_size() << "\n";
    os << level << "\tcapa_bits:" << capa_bits() << "\n";
    os << level << "\tsymb_size:" << symb_size() << "\n";
    os << level << "\tsymb_bits:" << symb_bits() << "\n";
    POPLAR_EX_STATS(
      os << level << "\tnum_resize:" << num_resize_ << "\n";
    )
  }

  void swap(HashTriePR& rhs) {
    std::swap(table_, rhs.table_);
    std::swap(size_, rhs.size_);
    std::swap(max_size_, rhs.max_size_);
    std::swap(capa_size_, rhs.capa_size_);
    std::swap(symb_size_, rhs.symb_size_);
    POPLAR_EX_STATS(
      std::swap(num_resize_, rhs.num_resize_);
    )
  }

  HashTriePR(const HashTriePR&) = delete;
  HashTriePR& operator=(const HashTriePR&) = delete;

  HashTriePR(HashTriePR&& rhs) noexcept : HashTriePR() {
    this->swap(rhs);
  }
  HashTriePR& operator=(HashTriePR&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  IntVector table_{};
  uint64_t size_{}; // # of registered nodes
  uint64_t max_size_{}; // t_factor% of the capacity
  size_p2_t capa_size_{};
  size_p2_t symb_size_{};

  POPLAR_EX_STATS(
    uint64_t num_resize_{};
  )

  uint64_t make_key_(uint64_t node_id, uint64_t symb) const {
    return (node_id << symb_size_.bits()) | symb;
  }
  uint64_t right_(uint64_t slot_id) const {
    return (slot_id + 1) & capa_size_.mask();
  }
};

} //ns - poplar

#endif //POPLAR_TRIE_HASH_TRIE_PR_HPP
