#ifndef POPLAR_TRIE_BIT_VECTOR_HPP
#define POPLAR_TRIE_BIT_VECTOR_HPP

#include <vector>

#include "bit_tools.hpp"

namespace poplar {

class BitVector {
public:
  BitVector() = default;

  explicit BitVector(uint64_t size) {
    size_ = size;
    chunks_.resize(size_ / 64 + 1);
  }

  ~BitVector() = default;

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

  void swap(BitVector& rhs) {
    std::swap(chunks_, rhs.chunks_);
    std::swap(size_, rhs.size_);
  }

  BitVector(const BitVector&) = delete;
  BitVector& operator=(const BitVector&) = delete;

  BitVector(BitVector&& rhs) noexcept : BitVector() {
    this->swap(rhs);
  }
  BitVector& operator=(BitVector&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  std::vector<uint64_t> chunks_{};
  uint64_t size_{};
};

} //ns - poplar

#endif //POPLAR_TRIE_BIT_VECTOR_HPP
