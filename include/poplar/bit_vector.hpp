#ifndef POPLAR_TRIE_BIT_VECTOR_HPP
#define POPLAR_TRIE_BIT_VECTOR_HPP

#include <vector>

#include "bit_tools.hpp"

namespace poplar {

class bit_vector {
 public:
  bit_vector() = default;

  explicit bit_vector(uint64_t size) {
    size_ = size;
    chunks_.resize(size_ / 64 + 1);
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

  uint64_t size() const {
    return size_;
  }

  bit_vector(const bit_vector&) = delete;
  bit_vector& operator=(const bit_vector&) = delete;

  bit_vector(bit_vector&& rhs) noexcept = default;
  bit_vector& operator=(bit_vector&& rhs) noexcept = default;

 private:
  std::vector<uint64_t> chunks_{};
  uint64_t size_{};
};

}  // namespace poplar

#endif  // POPLAR_TRIE_BIT_VECTOR_HPP
