#ifndef POPLAR_TRIE_COMPACT_VECTOR_HPP
#define POPLAR_TRIE_COMPACT_VECTOR_HPP

#include <vector>

#include "basics.hpp"

namespace poplar {

class compact_vector {
 public:
  compact_vector() = default;

  compact_vector(uint64_t size, uint32_t width) {
    assert(width < 64);

    size_ = size;
    mask_ = (1ULL << width) - 1;
    width_ = width;
    chunks_.resize(size_ * width_ / 64 + 1, 0);
  }

  compact_vector(uint64_t size, uint32_t width, uint64_t init) : compact_vector{size, width} {
    for (uint64_t i = 0; i < size; ++i) {
      set(i, init);
    }
  }

  ~compact_vector() = default;

  void resize(uint64_t size) {
    size_ = size;
    chunks_.resize(size_ * width_ / 64 + 1);
  }

  uint64_t operator[](uint64_t i) const {
    return get(i);
  }

  uint64_t get(uint64_t i) const {
    assert(i < size_);

    auto [quo, mod] = decompose_value<64>(i * width_);

    if (mod + width_ <= 64) {
      return (chunks_[quo] >> mod) & mask_;
    } else {
      return ((chunks_[quo] >> mod) | (chunks_[quo + 1] << (64 - mod))) & mask_;
    }
  }

  void set(uint64_t i, uint64_t v) {
    assert(i < size_);
    assert(v <= mask_);

    auto [quo, mod] = decompose_value<64>(i * width_);

    chunks_[quo] &= ~(mask_ << mod);
    chunks_[quo] |= (v & mask_) << mod;

    if (64 < mod + width_) {
      const uint64_t diff = 64 - mod;
      chunks_[quo + 1] &= ~(mask_ >> diff);
      chunks_[quo + 1] |= (v & mask_) >> diff;
    }
  }

  uint64_t size() const {
    return size_;
  }

  uint32_t width() const {
    return width_;
  }

  compact_vector(const compact_vector&) = delete;
  compact_vector& operator=(const compact_vector&) = delete;

  compact_vector(compact_vector&&) noexcept = default;
  compact_vector& operator=(compact_vector&&) noexcept = default;

 private:
  std::vector<uint64_t> chunks_;
  uint64_t size_ = 0;
  uint64_t mask_ = 0;
  uint64_t width_ = 0;
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_VECTOR_HPP
