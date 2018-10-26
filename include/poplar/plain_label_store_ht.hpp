#ifndef POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
#define POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP

#include <vector>

#include "basics.hpp"
#include "exception.hpp"

namespace poplar {

template <typename Value>
class plain_label_store_ht {
 public:
  using value_type = Value;

  static constexpr auto trie_type = trie_types::HASH_TRIE;
  static constexpr uint64_t page_size = 1U << 16;
  static constexpr uint64_t growth_size = 1U << 16;

 public:
  plain_label_store_ht() = default;

  explicit plain_label_store_ht(uint32_t capa_bits) {
    ptrs_.reserve(1ULL << capa_bits);
    ptrs_.emplace_back(0);
  }

  ~plain_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos + 1 < ptrs_.size());
    assert(pos / page_size < char_pages_.size());

    uint64_t cur_ptr = ptrs_[pos];
    if ((pos % page_size) == 0) {
      cur_ptr = 0;  // then ptrs_[pos] == char_pages_[pos / page_size - 1].size()
    }

    const uint8_t* char_ptr = char_pages_[pos / page_size].data() + cur_ptr;
    uint64_t alloc = ptrs_[pos + 1] - cur_ptr;

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
  }

  value_type* append(char_range key) {
    const uint64_t page_id = (ptrs_.size() - 1) / page_size;
    if (char_pages_.size() <= page_id) {
      char_pages_.emplace_back();
    }
    std::vector<uint8_t>& chars = char_pages_[page_id];

    uint64_t length = key.empty() ? 0 : key.length() - 1;
    for (uint64_t i = 0; i < length; ++i) {
      append_with_fixed_growth<growth_size>(chars, key.begin[i]);
    }

#ifdef POPLAR_EXTRA_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    const size_t vpos = chars.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      append_with_fixed_growth<growth_size>(chars, uint8_t(0));
    }

    POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
    ptrs_.emplace_back(chars.size());
    return reinterpret_cast<value_type*>(chars.data() + vpos);
  }

  void append_dummy() {
    const uint64_t page_id = (ptrs_.size() - 1) / page_size;
    if (char_pages_.size() <= page_id) {
      char_pages_.emplace_back();
    }
    std::vector<uint8_t>& chars = char_pages_[page_id];

    POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
    ptrs_.emplace_back(chars.size());
  }

  uint64_t size() const {
    return ptrs_.size() - 1;
  }

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "plain_label_store_ht");
    show_stat(os, indent, "size", size());
#ifdef POPLAR_EXTRA_STATS
    show_stat(os, indent, "max_length", max_length_);
    show_stat(os, indent, "ave_length", double(sum_length_) / size());
#endif
  }

  plain_label_store_ht(const plain_label_store_ht&) = delete;
  plain_label_store_ht& operator=(const plain_label_store_ht&) = delete;

  plain_label_store_ht(plain_label_store_ht&&) noexcept = default;
  plain_label_store_ht& operator=(plain_label_store_ht&&) noexcept = default;

 private:
  std::vector<std::vector<uint8_t>> char_pages_;
  std::vector<uint32_t> ptrs_;
#ifdef POPLAR_EXTRA_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
