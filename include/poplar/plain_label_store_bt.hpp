#ifndef POPLAR_TRIE_PLAIN_LABEL_STORE_BT_HPP
#define POPLAR_TRIE_PLAIN_LABEL_STORE_BT_HPP

#include <memory>
#include <vector>

#include "basics.hpp"
#include "compact_vector.hpp"

namespace poplar {

template <typename Value>
class plain_label_store_bt {
 public:
  using value_type = Value;

  static constexpr auto trie_type = trie_types::BONSAI_TRIE;

 public:
  plain_label_store_bt() = default;

  explicit plain_label_store_bt(uint32_t capa_bits) : ptrs_(1ULL << capa_bits) {}

  ~plain_label_store_bt() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(ptrs_[pos]);

    const uint8_t* ptr = ptrs_[pos].get();

    if (key.empty()) {
      return {reinterpret_cast<const value_type*>(ptr), 0};
    }

    for (uint64_t i = 0; i < key.length(); ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    return {reinterpret_cast<const value_type*>(ptr + key.length()), key.length()};
  }

  value_type* insert(uint64_t pos, char_range key) {
    assert(!ptrs_[pos]);

    ++size_;

    uint64_t length = key.length();
    ptrs_[pos] = std::make_unique<uint8_t[]>(length + sizeof(value_type));
    auto ptr = ptrs_[pos].get();
    copy_bytes(ptr, key.begin, length);

#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    auto ret = reinterpret_cast<value_type*>(ptr + length);
    *ret = static_cast<value_type>(0);

    return ret;
  }

  template <typename T>
  void expand(const T& pos_map) {
    std::vector<std::unique_ptr<uint8_t[]>> new_ptrs(capa_size() * 2);
    for (uint64_t i = 0; i < pos_map.size(); ++i) {
      if (pos_map[i] != UINT64_MAX) {
        new_ptrs[pos_map[i]] = std::move(ptrs_[i]);
      }
    }
    ptrs_ = std::move(new_ptrs);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t capa_size() const {
    return ptrs_.size();
  }

  boost::property_tree::ptree make_ptree() const {
    boost::property_tree::ptree pt;
    pt.put("name", "plain_label_store_bt");
    pt.put("size", size());
    pt.put("capa_size", capa_size());
#ifdef POPLAR_ENABLE_EX_STATS
    pt.put("max_length", max_length_);
    pt.put("ave_length", double(sum_length_) / size());
#endif
    return pt;
  }

  plain_label_store_bt(const plain_label_store_bt&) = delete;
  plain_label_store_bt& operator=(const plain_label_store_bt&) = delete;

  plain_label_store_bt(plain_label_store_bt&&) noexcept = default;
  plain_label_store_bt& operator=(plain_label_store_bt&&) noexcept = default;

 private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_;
  uint64_t size_ = 0;
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_BT_HPP
