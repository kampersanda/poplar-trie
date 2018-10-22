#ifndef POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP
#define POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "sparse_set.hpp"

namespace poplar {

template <typename Value>
class compact_label_store_ht {
 public:
  using this_type = compact_label_store_ht<Value>;
  using value_type = Value;

  static constexpr auto trie_type = trie_types::HASH_TRIE;

 public:
  compact_label_store_ht() = default;

  explicit compact_label_store_ht(uint32_t capa_bits) {
    chars_.reserve(1ULL << capa_bits);
    ptrs_.append(0);
  }

  ~compact_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos + 1 < ptrs_.size());

    auto [beg, end] = ptrs_.access_pair(pos);

    const uint8_t* char_ptr = chars_.data() + beg;
    uint64_t alloc = end - beg;

    if (key.empty()) {
      assert(sizeof(value_type) == alloc);
      return {reinterpret_cast<const value_type*>(char_ptr), 0};
    }

    assert(sizeof(value_type) <= alloc);

    uint64_t length = alloc - sizeof(value_type);
    for (uint64_t i = 0; i < length; ++i) {
      if (key[i] != char_ptr[i]) {
        return {nullptr, i};
      }
    }

    if (key[length] != '\0') {
      return {nullptr, length};
    }

    return {reinterpret_cast<const value_type*>(char_ptr + length), length + 1};
  };

  value_type* append(char_range key) {
    uint64_t length = key.empty() ? 0 : key.length() - 1;
    std::copy(key.begin, key.begin + length, std::back_inserter(chars_));

    max_length_ = std::max(max_length_, length);
    sum_length_ += length;

    const size_t vpos = chars_.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      chars_.emplace_back(0);
    }
    ptrs_.append(chars_.size());

    return reinterpret_cast<value_type*>(chars_.data() + vpos);
  }

  // Associate a dummy label
  void append_dummy() {
    chars_.emplace_back(0);
    ptrs_.append(chars_.size());
  }

  uint64_t size() const {
    return ptrs_.size() - 1;
  }
  uint64_t capa_size() const {
    return 0;
  }

  boost::property_tree::ptree make_ptree() const {
    boost::property_tree::ptree pt;
    pt.put("name", "compact_label_store_ht");
    pt.put("size", size());
    pt.put("capa_size", capa_size());
#ifdef POPLAR_ENABLE_EX_STATS
    pt.put("max_length", max_length_);
    pt.put("ave_length", double(sum_length_) / size());
#endif
    return pt;
  }

  compact_label_store_ht(const compact_label_store_ht&) = delete;
  compact_label_store_ht& operator=(const compact_label_store_ht&) = delete;

  compact_label_store_ht(compact_label_store_ht&&) noexcept = default;
  compact_label_store_ht& operator=(compact_label_store_ht&&) noexcept = default;

 private:
  std::vector<uint8_t> chars_;
  sparse_set ptrs_;
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP
