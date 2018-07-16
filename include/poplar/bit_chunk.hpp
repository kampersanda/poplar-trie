#ifndef POPLAR_TRIE_BIT_CHUNK_HPP
#define POPLAR_TRIE_BIT_CHUNK_HPP

#include "bit_tools.hpp"

namespace poplar {

template <uint64_t ChunkSize>
class bit_chunk {
 private:
  static_assert(ChunkSize > 64 and is_power2(ChunkSize));

 public:
  static constexpr uint64_t chunk_size = ChunkSize;

  bit_chunk() = default;

  bool get(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::get_bit(chunks_[i / 64], i % 64);
  }
  void set(uint64_t i) {
    assert(i < chunk_size);
    bit_tools::set_bit(chunks_[i / 64], i % 64);
  }
  uint64_t popcnt() const {
    uint64_t sum = 0;
    for (auto chunk : chunks_) {
      sum += bit_tools::popcnt(chunk);
    }
    return sum;
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < chunk_size);
    uint64_t sum = 0, j = i / 64;
    for (uint64_t k = 0; k < j; ++k) {
      sum += bit_tools::popcnt(chunks_[k]);
    }
    return sum + bit_tools::popcnt(chunks_[j], i % 64);
  }

 private:
  uint64_t chunks_[chunk_size / 64] = {};
};

template <>
class bit_chunk<64> {
 public:
  static constexpr uint64_t chunk_size = 64;

  bit_chunk() = default;

  bool get(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < chunk_size);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint64_t chunk_{};
};

template <>
class bit_chunk<32> {
 public:
  static constexpr uint64_t chunk_size = 32;

  bit_chunk() = default;

  bool get(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < chunk_size);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint32_t chunk_{};
};

template <>
class bit_chunk<16> {
 public:
  static constexpr uint64_t chunk_size = 16;

  bit_chunk() = default;

  bool get(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < chunk_size);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint16_t chunk_{};
};

template <>
class bit_chunk<8> {
 public:
  static constexpr uint64_t chunk_size = 8;

  bit_chunk() = default;

  bool get(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::get_bit(chunk_, i);
  }
  void set(uint64_t i) {
    assert(i < chunk_size);
    bit_tools::set_bit(chunk_, i);
  }
  uint64_t popcnt() const {
    return bit_tools::popcnt(chunk_);
  }
  uint64_t popcnt(uint64_t i) const {
    assert(i < chunk_size);
    return bit_tools::popcnt(chunk_, i);
  }

 private:
  uint8_t chunk_{};
};

}  // namespace poplar

#endif  // POPLAR_TRIE_BIT_CHUNK_HPP
