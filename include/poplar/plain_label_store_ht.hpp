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

 public:
  plain_label_store_ht() = default;

  explicit plain_label_store_ht(uint32_t capa_bits) {
    ptrs_.reserve(1ULL << capa_bits);
    ptrs_.emplace_back(0);
  }

  ~plain_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos + 1 < ptrs_.size());

    const uint8_t* char_ptr = chars_[pos / page_size].data() + ptrs_[pos];
    uint64_t alloc = ptrs_[pos + 1] - ptrs_[pos];

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
    const uint64_t pos = ptrs_.size() - 1;
    const uint64_t page_id = pos / page_size;
    if (chars_.size() <= page_id) {
      chars_.back().shrink_to_fit();
      chars_.emplace_back();
    }

    std::vector<uint8_t>& chars = chars_[page_id];

    uint64_t length = key.empty() ? 0 : key.length() - 1;
    std::copy(key.begin, key.begin + length, std::back_inserter(chars));

    max_length_ = std::max(max_length_, length);
    sum_length_ += length;

    const size_t vpos = chars.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      chars.emplace_back(0);
    }

    POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
    ptrs_.emplace_back(chars.size());
    return reinterpret_cast<value_type*>(chars.data() + vpos);
  }

  void append_dummy() {
    const uint64_t pos = ptrs_.size() - 1;
    const uint64_t page_id = pos / page_size;
    if (chars_.size() <= page_id) {
      chars_.back().shrink_to_fit();
      chars_.emplace_back();
    }
    std::vector<uint8_t>& chars = chars_[page_id];

    POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
    ptrs_.emplace_back(chars.size());
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
    show_stat(os, indent, "name", "plain_label_store_ht");
    show_stat(os, indent, "size", size());
    show_stat(os, indent, "max_length", max_length());
    show_stat(os, indent, "ave_length", ave_length());
  }

  plain_label_store_ht(const plain_label_store_ht&) = delete;
  plain_label_store_ht& operator=(const plain_label_store_ht&) = delete;

  plain_label_store_ht(plain_label_store_ht&&) noexcept = default;
  plain_label_store_ht& operator=(plain_label_store_ht&&) noexcept = default;

 private:
  std::vector<std::vector<uint8_t>> chars_;
  std::vector<uint32_t> ptrs_;
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
