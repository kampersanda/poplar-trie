#ifndef POPLAR_TRIE_BIT_VECTOR_HPP
#define POPLAR_TRIE_BIT_VECTOR_HPP

#include <vector>

#include "bit_tools.hpp"

namespace poplar {

class bit_vector {
 public:
  bit_vector() = default;

  explicit bit_vector(uint64_t size) {
    chunks_.resize(bit_tools::words_for(size));
    size_ = size;
  }

  ~bit_vector() = default;

  bool operator[](uint64_t i) const {
    return get(i);
  }
  bool get(uint64_t i) const {
    assert(i < size_);
    return bit_tools::get_bit(chunks_[i / 64], i % 64);
  }
  void set(uint64_t i, bool bit = true) {
    assert(i < size_);
    bit_tools::set_bit(chunks_[i / 64], i % 64, bit);
  }

  uint64_t get_bits(uint64_t pos, uint32_t len) const {
    assert(pos + len <= size());
    if (len == 0) {
      return 0;
    }
    uint64_t chunk_id = pos / 64;
    uint64_t pos_in_chunk = pos % 64;
    uint64_t mask = -(len == 64) | ((1ULL << len) - 1);
    if (pos_in_chunk + len <= 64) {
      return (chunks_[chunk_id] >> pos_in_chunk) & mask;
    } else {
      return (chunks_[chunk_id] >> pos_in_chunk) | ((chunks_[chunk_id + 1] << (64 - pos_in_chunk)) & mask);
    }
  }

  void append_bit(bool bit) {
    uint64_t pos_in_chunk = size_ % 64;
    if (pos_in_chunk == 0) {
      chunks_.emplace_back(0);
    }
    chunks_.back() |= static_cast<uint64_t>(bit) << pos_in_chunk;
    ++size_;
  }

  void append_bits(uint64_t bits, uint32_t len) {
    assert(len == 64 or (bits >> len) == 0);

    if (len == 0) {
      return;
    }
    uint64_t pos_in_chunk = len % 64;
    size_ += len;
    if (pos_in_chunk == 0) {
      chunks_.push_back(bits);
    } else {
      chunks_.back() |= (bits << pos_in_chunk);
      if (len > 64 - pos_in_chunk) {
        chunks_.push_back(bits >> (64 - pos_in_chunk));
      }
    }
  }

  uint64_t size() const {
    return size_;
  }

  bit_vector(const bit_vector&) = delete;
  bit_vector& operator=(const bit_vector&) = delete;

  bit_vector(bit_vector&& rhs) noexcept = default;
  bit_vector& operator=(bit_vector&& rhs) noexcept = default;

 private:
  std::vector<uint64_t> chunks_;
  uint64_t size_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_BIT_VECTOR_HPP
