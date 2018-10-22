#ifndef POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP
#define POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "vbyte.hpp"

namespace poplar {

template <typename Value, uint64_t ChunkSize = 16>
class compact_label_store_ht {
 public:
  using this_type = compact_label_store_ht<Value, ChunkSize>;
  using value_type = Value;
  using chunk_type = typename chunk_type_traits<ChunkSize>::type;

  static constexpr auto trie_type = trie_types::HASH_TRIE;
  static constexpr uint64_t page_size = 1U << 16;

 public:
  compact_label_store_ht() = default;

  explicit compact_label_store_ht(uint32_t capa_bits) {
    // chars_.reserve((1ULL << capa_bits) * sizeof(value_type));
    ptrs_.reserve((1ULL << capa_bits) / ChunkSize);
  }

  ~compact_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos < size_);
    assert(pos / page_size < char_pages_.size());

    auto [group_id, offset] = decompose_value<ChunkSize>(pos);
    auto char_ptr = char_pages_[pos / page_size].data() + ptrs_[group_id];

    uint64_t alloc = 0;
    for (uint64_t i = 0; i < offset; ++i) {
      char_ptr += vbyte::decode(char_ptr, alloc);
      char_ptr += alloc;
    }
    char_ptr += vbyte::decode(char_ptr, alloc);

    if (key.empty()) {
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

    // +1 considers the terminator '\0'
    return {reinterpret_cast<const value_type*>(char_ptr + length), length + 1};
  };

  value_type* append(char_range key) {
    const uint64_t page_id = size_ / page_size;
    if (char_pages_.size() <= page_id) {
      if (!char_pages_.empty()) {
        // char_pages_.back().shrink_to_fit();
      }
      char_pages_.emplace_back();
    }
    std::vector<uint8_t>& chars = char_pages_[page_id];

    auto [group_id, offset] = decompose_value<ChunkSize>(size_++);
    if (offset == 0) {
      POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
      ptrs_.push_back(chars.size());
    }

    uint64_t length = key.empty() ? 0 : key.length() - 1;
    vbyte::append(chars, length + sizeof(value_type));
    std::copy(key.begin, key.begin + length, std::back_inserter(chars));

    max_length_ = std::max<uint64_t>(max_length_, key.length());
    sum_length_ += key.length();

    const size_t vpos = chars.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      chars.emplace_back(0);
    }

    return reinterpret_cast<value_type*>(chars.data() + vpos);
  }

  // Associate a dummy label
  void append_dummy() {
    const uint64_t page_id = size_ / page_size;
    if (char_pages_.size() <= page_id) {
      if (!char_pages_.empty()) {
        // char_pages_.back().shrink_to_fit();
      }
      char_pages_.emplace_back();
    }
    std::vector<uint8_t>& chars = char_pages_[page_id];

    auto [group_id, offset] = decompose_value<ChunkSize>(size_++);
    if (offset == 0) {
      POPLAR_THROW_IF(UINT32_MAX < chars.size(), "ptr value overflows.");
      ptrs_.push_back(chars.size());
    }
    vbyte::append(chars, 0);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t num_ptrs() const {
    return ptrs_.size();
  }
  uint64_t max_length() const {
    return max_length_;
  }
  double ave_length() const {
    return double(sum_length_) / size();
  }

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "compact_label_store_ht");
    show_stat(os, indent, "size", size());
    show_stat(os, indent, "num_ptrs", num_ptrs());
    show_stat(os, indent, "max_length", max_length());
    show_stat(os, indent, "ave_length", ave_length());
    show_stat(os, indent, "chunk_size", ChunkSize);
  }

  compact_label_store_ht(const compact_label_store_ht&) = delete;
  compact_label_store_ht& operator=(const compact_label_store_ht&) = delete;

  compact_label_store_ht(compact_label_store_ht&&) noexcept = default;
  compact_label_store_ht& operator=(compact_label_store_ht&&) noexcept = default;

 private:
  std::vector<std::vector<uint8_t>> char_pages_;
  std::vector<uint32_t> ptrs_;
  uint64_t size_ = 0;
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_LABEL_STORE_HT_HPP
