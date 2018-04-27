#ifndef POPLAR_TRIE_INT_VECTOR_HPP
#define POPLAR_TRIE_INT_VECTOR_HPP

#include <vector>

#include "basics.hpp"

namespace poplar {

class IntVector {
 public:
  IntVector() = default;

  IntVector(uint64_t size, uint32_t width) {
    assert(width < 64);

    size_ = size;
    mask_ = (1ULL << width) - 1;
    width_ = width;
    chunks_.resize(size_ * width_ / 64 + 1);
  }

  IntVector(uint64_t size, uint32_t width, uint64_t init) : IntVector{size, width} {
    for (uint64_t i = 0; i < size; ++i) {
      set(i, init);
    }
  }

  ~IntVector() = default;

  void resize(uint64_t size) {
    size_ = size;
    chunks_.resize(size_ * width_ / 64 + 1);
  }

  uint64_t operator[](uint64_t i) const {
    return get(i);
  }

  uint64_t get(uint64_t i) const {
    assert(i < size_);

    decomp_val_t dec = decompose_value<64>(i * width_);

    if (dec.mod + width_ <= 64) {
      return (chunks_[dec.quo] >> dec.mod) & mask_;
    } else {
      return ((chunks_[dec.quo] >> dec.mod) | (chunks_[dec.quo + 1] << (64 - dec.mod))) & mask_;
    }
  }

  void set(uint64_t i, uint64_t v) {
    assert(i < size_);
    assert(v <= mask_);

    decomp_val_t dec = decompose_value<64>(i * width_);

    chunks_[dec.quo] &= ~(mask_ << dec.mod);
    chunks_[dec.quo] |= (v & mask_) << dec.mod;

    if (64 < dec.mod + width_) {
      const uint64_t diff = 64 - dec.mod;
      chunks_[dec.quo + 1] &= ~(mask_ >> diff);
      chunks_[dec.quo + 1] |= (v & mask_) >> diff;
    }
  }

  uint64_t size() const {
    return size_;
  }

  uint32_t width() const {
    return width_;
  }

  void swap(IntVector& rhs) {
    std::swap(chunks_, rhs.chunks_);
    std::swap(size_, rhs.size_);
    std::swap(mask_, rhs.mask_);
    std::swap(width_, rhs.width_);
  }

  IntVector(const IntVector&) = delete;
  IntVector& operator=(const IntVector&) = delete;

  IntVector(IntVector&& rhs) noexcept : IntVector() {
    this->swap(rhs);
  }
  IntVector& operator=(IntVector&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

 private:
  std::vector<uint64_t> chunks_{};
  uint64_t size_{};
  uint64_t mask_{};
  uint32_t width_{};
};

}  // namespace poplar

#endif  // POPLAR_TRIE_INT_VECTOR_HPP
