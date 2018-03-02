#ifndef POPLAR_TRIE_LABEL_STORE_EM_HPP
#define POPLAR_TRIE_LABEL_STORE_EM_HPP

#include <memory>
#include <vector>

#include "BitChunk.hpp"

namespace poplar {

template<typename t_value, typename t_chunk>
class LabelStoreEM {
public:
  using ls_type = LabelStoreEM<t_value, t_chunk>;
  using value_type = t_value;
  using chunk_type = t_chunk;

  static constexpr uint64_t CHUNK_SIZE = chunk_type::SIZE;

public:
  LabelStoreEM() = default;

  explicit LabelStoreEM(uint32_t capa_bits)
    : ptrs_(1ULL << capa_bits), vptrs_(ptrs_.size() / CHUNK_SIZE), chunks_(vptrs_.size()) {}

  ~LabelStoreEM() {
    for (uint64_t i = 0; i < ptrs_.size(); ++i) {
      decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(i);
      if (!chunks_[pos_c.quo].get(pos_c.mod) && ptrs_[i].ptr != nullptr) {
        delete[] ptrs_[i].ptr;
      }
    }
  }

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, const ustr_view& key) const {
    const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);
    const chunk_type& chunk = chunks_[pos_c.quo];

    if (key.empty()) {
      // Return the target value pointer without comparison
      if (chunk.get(pos_c.mod)) {
        // For a short label
        return {&vptrs_[pos_c.quo][chunk.popcnt(pos_c.mod)], 0};
      } else {
        // For a long label
        assert(ptrs_[pos].ptr != nullptr);
        return {reinterpret_cast<const value_type*>(ptrs_[pos].ptr), 0};
      }
    }

    uint64_t i = 0; // #match
    const value_type* ret = nullptr;

    if (chunk.get(pos_c.mod)) {
      // For a short label
      const uint8_t* str = ptrs_[pos].str;
      auto len = std::min<uint64_t>(key.length(), sizeof(void*));
      for (;i < len; ++i) {
        if (key[i] != str[i]) {
          return {nullptr, i};
        }
      }
      if (i == key.length()) {
        // nothing to do
      } else if (key[i] == '\0') {
        ++i; // For the omitted terminator
      } else {
        return {nullptr, i};
      }
      ret = &vptrs_[pos_c.quo][chunk.popcnt(pos_c.mod)];
    } else {
      // For a long label
      const uint8_t* ptr = ptrs_[pos].ptr;
      for (; i < key.length(); ++i) {
        if (key[i] != ptr[i]) {
          return {nullptr, i};
        }
      }
      ret = reinterpret_cast<const t_value*>(ptr + i);
    }

    return {ret, i};
  }

  t_value* associate(uint64_t pos, const ustr_view& key) {
    const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);
    const chunk_type& chunk = chunks_[pos_c.quo];

    assert(!chunk.get(pos_c.mod));

    value_type* ret = nullptr;
    uint64_t length = key.length();

    if (length <= sizeof(void*) + 1) {
      // For a short label
      copy_bytes(ptrs_[pos].str, key.data(), std::min<uint64_t>(length, sizeof(void*)));
      ret = insert_value_(pos_c);
    } else {
      // For a long label
      ptrs_[pos].ptr = new uint8_t[length + sizeof(value_type)]; // TODO: Remove new operation
      copy_bytes(ptrs_[pos].ptr, key.data(), length);
      ret = reinterpret_cast<value_type*>(ptrs_[pos].ptr + length);
    }

    ++size_;

    POPLAR_EX_STATS(
      max_length_ = std::max(max_length_, length);
      sum_length_ += length;
    )

    *ret = static_cast<value_type>(0); // initialization
    return ret;
  }

  template <typename T>
  void expand(const T& pos_map) {
    ls_type new_ls(bit_tools::get_num_bits(capa_size()));

    for (uint64_t pos = 0; pos < pos_map.size(); ++pos) {
      uint64_t new_pos = pos_map[pos];

      if (new_pos != UINT64_MAX) {
        new_ls.ptrs_[new_pos] = ptrs_[pos];
        const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);

        if (chunks_[pos_c.quo].get(pos_c.mod)) {
          const decomp_val_t new_pos_c = decompose_value<CHUNK_SIZE>(new_pos);
          value_type* v = new_ls.insert_value_(new_pos_c);
          *v = vptrs_[pos_c.quo][chunks_[pos_c.quo].popcnt(pos_c.mod)];
        }
      }
    }

    new_ls.size_ = size_;
    POPLAR_EX_STATS(
      new_ls.max_length_ = max_length_;
      new_ls.sum_length_ = sum_length_;
    )
    ptrs_.clear(); // to avoid free() in destructor
    *this = std::move(new_ls);
  }

  uint64_t size() const {
    return size_;
  }

  uint64_t capa_size() const {
    return ptrs_.size();
  }

  void show_stat(std::ostream& os, std::string&& level = "") const {
    os << level << "stat:LabelStoreEM\n";
    os << level << "\tchunk_size:" << CHUNK_SIZE << "\n";
    os << level << "\tsize:" << size() << "\n";
    os << level << "\tcapa_size:" << capa_size() << "\n";
    POPLAR_EX_STATS(
      os << level << "\tmax_length:" << max_length_ << "\n";
      os << level << "\tave_length:" << double(sum_length_) / size() << "\n";
    )
  }

  void swap(LabelStoreEM& rhs) {
    std::swap(ptrs_, rhs.ptrs_);
    std::swap(vptrs_, rhs.vptrs_);
    std::swap(chunks_, rhs.chunks_);
    std::swap(size_, rhs.size_);
    POPLAR_EX_STATS(
      std::swap(max_length_, rhs.max_length_);
      std::swap(sum_length_, rhs.sum_length_);
    )
  }

  LabelStoreEM(const LabelStoreEM&) = delete;
  LabelStoreEM& operator=(const LabelStoreEM&) = delete;

  LabelStoreEM(LabelStoreEM&& rhs) noexcept : LabelStoreEM() {
    this->swap(rhs);
  }
  LabelStoreEM& operator=(LabelStoreEM&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  union ptr_t {
    uint8_t* ptr = nullptr; // for long labels
    uint8_t str[sizeof(void*)]; // for short labels
  };
  std::vector<ptr_t> ptrs_{};
  std::vector<std::unique_ptr<value_type[]>> vptrs_{}; // for each chunk
  std::vector<chunk_type> chunks_{}; // chunks of bitmap
  uint64_t size_{};

  POPLAR_EX_STATS(
    uint64_t max_length_{};
    uint64_t sum_length_{};
  )

  // Allocates a value area for short labels in O(CHUNK_SIZE) time
  value_type* insert_value_(const decomp_val_t& pos_c) {
    chunk_type& chunk = chunks_[pos_c.quo];
    assert(!chunk.get(pos_c.mod));

    uint64_t num = chunk.popcnt();

    value_type* ret = nullptr;
    auto new_vptr = std::make_unique<value_type[]>(num + 1);

    if (num == 0) {
      // initial
      assert(!vptrs_[pos_c.quo]);
      vptrs_[pos_c.quo] = std::move(new_vptr);
      ret = vptrs_[pos_c.quo].get();
    } else {
      uint64_t offset = chunk.popcnt(pos_c.mod);
      const value_type* vptr = vptrs_[pos_c.quo].get();

      uint64_t i = 0;
      for (; i < offset; ++i) {
        new_vptr[i] = vptr[i];
      }
      ret = &new_vptr[i];
      for (; i < num; ++i) {
        new_vptr[i + 1] = vptr[i];
      }
      vptrs_[pos_c.quo] = std::move(new_vptr);
    }

    chunk.set(pos_c.mod);
    return ret;
  }
};

} //ns - poplar

#endif //POPLAR_TRIE_LABEL_STORE_EM_HPP
