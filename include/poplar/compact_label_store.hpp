#ifndef POPLAR_TRIE_COMPACT_LABEL_STORE_HPP
#define POPLAR_TRIE_COMPACT_LABEL_STORE_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "bit_chunk.hpp"
#include "vbyte.hpp"

namespace poplar {

template <typename Value, uint64_t ChunkSize = 16>
class compact_label_store {
 public:
  using this_type = compact_label_store<Value, ChunkSize>;
  using value_type = Value;

  static constexpr bool random_order = false;

 public:
  compact_label_store() = default;

  explicit compact_label_store(uint32_t capa_bits) {
    ptrs_.reserve((1ULL << capa_bits) / ChunkSize);
  }

  ~compact_label_store() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos < size_);

    auto [group_id, offset] = decompose_value<ChunkSize>(pos);
    auto ptr = group_id < ptrs_.size() ? ptrs_[group_id].get() : char_buf_.data();

    uint64_t alloc = 0;
    for (uint64_t i = 0; i < offset; ++i) {
      ptr += vbyte::decode(ptr, alloc);
      ptr += alloc;
    }
    ptr += vbyte::decode(ptr, alloc);

    if (key.empty()) {
      return {reinterpret_cast<const value_type*>(ptr), 0};
    }

    assert(sizeof(value_type) <= alloc);

    uint64_t length = alloc - sizeof(value_type);
    for (uint64_t i = 0; i < length; ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    if (key[length] != '\0') {
      return {nullptr, length};
    }

    // +1 considers the terminator '\0'
    return {reinterpret_cast<const value_type*>(ptr + length), length + 1};
  };

  value_type* associate(uint64_t pos, char_range key) {
    assert(pos == size_);

    ++size_;
    auto [group_id, offset] = decompose_value<ChunkSize>(pos);

    if (pos != 0 and offset == 0) {
      release_buffer_();
    }

#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max<uint64_t>(max_length_, key.length());
    sum_length_ += key.length();
#endif

    uint64_t length = key.empty() ? 0 : key.length() - 1;
    vbyte::append(char_buf_, length + sizeof(value_type));
    std::copy(key.begin, key.begin + length, std::back_inserter(char_buf_));

    const size_t vpos = char_buf_.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      char_buf_.emplace_back(0);
    }

    return reinterpret_cast<value_type*>(char_buf_.data() + vpos);
  }

  // Associate a dummy label
  void dummy_associate(uint64_t pos) {
    assert(pos == size_);

    ++size_;
    auto [group_id, offset] = decompose_value<ChunkSize>(pos);

    if (pos != 0 and offset == 0) {
      release_buffer_();
    }
    vbyte::append(char_buf_, 0);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t capa_size() const {
    return ptrs_.capacity() * ChunkSize;
  }

  boost::property_tree::ptree make_ptree() const {
    boost::property_tree::ptree pt;
    pt.put("name", "compact_label_store");
    pt.put("ChunkSize", ChunkSize);
    pt.put("size", size());
    pt.put("capa_size", capa_size());
#ifdef POPLAR_ENABLE_EX_STATS
    pt.put("max_length", max_length_);
    pt.put("ave_length", double(sum_length_) / size());
#endif
    return pt;
  }

  compact_label_store(const compact_label_store&) = delete;
  compact_label_store& operator=(const compact_label_store&) = delete;

  compact_label_store(compact_label_store&&) noexcept = default;
  compact_label_store& operator=(compact_label_store&&) noexcept = default;

 private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_;
  std::vector<uint8_t> char_buf_;
  uint64_t size_ = 0;

#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif

  void release_buffer_() {
    auto new_uptr = std::make_unique<uint8_t[]>(char_buf_.size());
    copy_bytes(new_uptr.get(), char_buf_.data(), char_buf_.size());
    ptrs_.emplace_back(std::move(new_uptr));
    char_buf_.clear();
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_LABEL_STORE_HPP
