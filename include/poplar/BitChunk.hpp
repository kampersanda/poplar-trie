#ifndef POPLAR_TRIE_BIT_CHUNK_HPP
#define POPLAR_TRIE_BIT_CHUNK_HPP

#include "bit_tools.hpp"

namespace poplar {

template <uint64_t t_size>
class BitChunk {
 private:
  static_assert(t_size > 64 && is_power2(t_size));

 public:
  static constexpr uint64_t SIZE = t_size;

  BitChunk() = default;

  bool get(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::get_bit(chunks_[i / 64], i % 64);
  }
  void set(uint64_t i) {
    assert(i < SIZE);
    bit_tools::set_bit(chunks_[i / 64], i % 64);
  }
  uint64_t popcnt() const {
    uint64_t sum = 0;
    for (const uint64_t chunk : chunks_) {
      sum += bit_tools::popcnt(chunk);
    }
    return sum;
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < SIZE);
    uint64_t sum = 0, j = i / 64;
    for (uint64_t k = 0; k < j; ++k) {
      sum += bit_tools::popcnt(chunks_[k]);
    }
    return sum + bit_tools::popcnt(chunks_[j], i % 64);
  }

 private:
  uint64_t chunks_[SIZE / 64] = {};
};

template <>
class BitChunk<64> {
 public:
  static constexpr uint64_t SIZE = 64;

  BitChunk() = default;

  bool get(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < SIZE);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint64_t chunk_{};
};

template <>
class BitChunk<32> {
 public:
  static constexpr uint64_t SIZE = 32;

  BitChunk() = default;

  bool get(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < SIZE);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint32_t chunk_{};
};

template <>
class BitChunk<16> {
 public:
  static constexpr uint64_t SIZE = 16;

  BitChunk() = default;

  bool get(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < SIZE);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint16_t chunk_{};
};

template <>
class BitChunk<8> {
 public:
  static constexpr uint64_t SIZE = 8;

  BitChunk() = default;

  bool get(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < SIZE);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < SIZE);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint8_t chunk_{};
};

}  // namespace poplar

#endif  // POPLAR_TRIE_BIT_CHUNK_HPP
