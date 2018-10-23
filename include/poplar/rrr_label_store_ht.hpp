#ifndef POPLAR_TRIE_RRR_LABEL_STORE_HT_HPP
#define POPLAR_TRIE_RRR_LABEL_STORE_HT_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "rrr_sparse_set.hpp"

namespace poplar {

template <typename Value>
class rrr_label_store_ht {
 public:
  using this_type = rrr_label_store_ht<Value>;
  using value_type = Value;

  static constexpr auto trie_type = trie_types::HASH_TRIE;

 public:
  rrr_label_store_ht() = default;

  explicit rrr_label_store_ht(uint32_t capa_bits) {
    uint64_t capa = (1ULL << capa_bits) * sizeof(value_type);
    chars_.reserve(capa);
    ptrs_.reserve(capa);
    ptrs_.append(0);
  }

  ~rrr_label_store_ht() = default;

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
  uint64_t max_length() const {
    return max_length_;
  }
  double ave_length() const {
    return double(sum_length_) / size();
  }

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "rrr_label_store_ht");
    show_stat(os, indent, "size", size());
    show_stat(os, indent, "max_length", max_length());
    show_stat(os, indent, "ave_length", ave_length());
    show_member(os, indent, "ptrs_");
    ptrs_.show_stats(os, n + 1);
  }

  rrr_label_store_ht(const rrr_label_store_ht&) = delete;
  rrr_label_store_ht& operator=(const rrr_label_store_ht&) = delete;

  rrr_label_store_ht(rrr_label_store_ht&&) noexcept = default;
  rrr_label_store_ht& operator=(rrr_label_store_ht&&) noexcept = default;

 private:
  std::vector<uint8_t> chars_;
  rrr_sparse_set ptrs_;
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_RRR_LABEL_STORE_HT_HPP
