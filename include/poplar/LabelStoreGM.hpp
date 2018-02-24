#ifndef POPLAR_TRIE_LABEL_STORE_GM_HPP
#define POPLAR_TRIE_LABEL_STORE_GM_HPP

#include <memory>
#include <vector>

#include "BitChunk.hpp"
#include "vbyte.hpp"

namespace poplar {

template <typename t_value, typename t_chunk>
class LabelStoreGM {
public:
  using ls_type = LabelStoreGM<t_value, t_chunk>;
  using value_type = t_value;
  using chunk_type = t_chunk;

  static constexpr uint64_t CHUNK_SIZE = chunk_type::SIZE;

public:
  LabelStoreGM() = default;

  explicit LabelStoreGM(uint32_t capa_bits)
    : ptrs_((1ULL << capa_bits) / CHUNK_SIZE), chunks_(ptrs_.size()) {}

  ~LabelStoreGM() = default;

  std::pair<const t_value*, uint64_t> compare(uint64_t pos, const ustr_view& key) const {
    const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);

    assert(ptrs_[pos_c.quo]);
    assert(chunks_[pos_c.quo].get(pos_c.mod));

    const uint8_t* ptr = ptrs_[pos_c.quo].get();
    const uint64_t offset = chunks_[pos_c.quo].popcnt(pos_c.mod);

    uint64_t alloc = 0;
    for (uint64_t i = 0; i < offset; ++i) {
      ptr += vbyte::decode(ptr, alloc);
      ptr += alloc;
    }
    ptr += vbyte::decode(ptr, alloc);

    if (key.empty()) {
      return {reinterpret_cast<const t_value*>(ptr), 0};
    }

    uint64_t length = alloc - sizeof(t_value);
    for (uint64_t i = 0; i < length; ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    if (key[length] != '\0') {
      return {nullptr, length};
    }

    // +1 considers the terminator '\0'
    return {reinterpret_cast<const t_value*>(ptr + length), length + 1};
  };

  t_value* associate(uint64_t pos, const ustr_view& key) {
    const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);

    assert(!chunks_[pos_c.quo].get(pos_c.mod));
    chunks_[pos_c.quo].set(pos_c.mod);

    ++size_;

    POPLAR_EX_STATS(
      max_length_ = std::max<uint64_t>(max_length_, key.length());
      sum_length_ += key.length();
    )

    if (!ptrs_[pos_c.quo]) {
      // First association in the group
      uint64_t length = key.empty() ? 0 : key.length() - 1;
      uint64_t new_alloc = vbyte::size(length + sizeof(value_type)) + length + sizeof(value_type);

      ptrs_[pos_c.quo] = std::make_unique<uint8_t[]>(new_alloc);
      uint8_t* ptr = ptrs_[pos_c.quo].get();

      ptr += vbyte::encode(ptr, length + sizeof(value_type));
      copy_bytes(ptr, key.data(), length);

      auto ret_ptr = reinterpret_cast<value_type*>(ptr + length);
      *ret_ptr = static_cast<value_type>(0);

      return ret_ptr;
    }

    // Second and subsequent association in the group
    auto fr_alloc = get_allocs_(pos_c);

    const uint64_t len = key.empty() ? 0 : key.length() - 1;
    const uint64_t new_alloc = vbyte::size(len + sizeof(value_type)) + len + sizeof(value_type);

    auto new_unique = std::make_unique<uint8_t[]>(fr_alloc.first + new_alloc + fr_alloc.second);

    // Get raw pointers
    const uint8_t* orig_ptr = ptrs_[pos_c.quo].get();
    uint8_t* new_ptr = new_unique.get();

    // Copy the front allocation
    copy_bytes(new_ptr, orig_ptr, fr_alloc.first);
    orig_ptr += fr_alloc.first;
    new_ptr += fr_alloc.first;

    // Set new allocation
    new_ptr += vbyte::encode(new_ptr, len + sizeof(value_type));
    copy_bytes(new_ptr, key.data(), len);
    new_ptr += len;
    *reinterpret_cast<value_type*>(new_ptr) = static_cast<value_type>(0);

    // Copy the back allocation
    copy_bytes(new_ptr + sizeof(value_type), orig_ptr, fr_alloc.second);

    // Overwrite
    ptrs_[pos_c.quo] = std::move(new_unique);

    return reinterpret_cast<value_type*>(new_ptr);
  }

  template <typename T>
  void expand(const T& pos_map) {
    ls_type new_ls(bit_tools::get_num_bits(capa_size()));

    for (uint64_t pos = 0; pos < pos_map.size(); ++pos) {
      decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);
      uint64_t new_pos = pos_map[pos];
      if (new_pos != UINT64_MAX) {
        auto orig_slice = get_slice_(pos_c);
        if (!orig_slice.empty()) {
          new_ls.set_slice_(decompose_value<CHUNK_SIZE>(new_pos), orig_slice);
        }
      }
      if (pos_c.mod == CHUNK_SIZE - 1) {
        ptrs_[pos_c.quo].reset();
      }
    }

    new_ls.size_ = size_;
    POPLAR_EX_STATS(
      new_ls.max_length_ = max_length_;
      new_ls.sum_length_ = sum_length_;
    )
    *this = std::move(new_ls);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t capa_size() const {
    return ptrs_.size() * CHUNK_SIZE;
  }

  void show_stat(std::ostream& os) const {
    os << "Statistics of LabelStoreGM\n";
    os << " - chunk_size: " << CHUNK_SIZE << "\n";
    os << " - size: " << size() << "\n";
    os << " - capa_size: " << capa_size() << "\n";
    POPLAR_EX_STATS(
      os << " - max_length: " << max_length_ << "\n";
      os << " - ave_length: " << double(sum_length_) / size() << "\n";
    )
  }

  void swap(LabelStoreGM& rhs) {
    std::swap(ptrs_, rhs.ptrs_);
    std::swap(chunks_, rhs.chunks_);
    std::swap(size_, rhs.size_);
    POPLAR_EX_STATS(
      std::swap(max_length_, rhs.max_length_);
      std::swap(sum_length_, rhs.sum_length_);
    )
  }

  LabelStoreGM(const LabelStoreGM&) = delete;
  LabelStoreGM& operator=(const LabelStoreGM&) = delete;

  LabelStoreGM(LabelStoreGM&& rhs) noexcept : LabelStoreGM() {
    this->swap(rhs);
  }
  LabelStoreGM& operator=(LabelStoreGM&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_{};
  std::vector<chunk_type> chunks_{};
  uint64_t size_{};

  POPLAR_EX_STATS(
    uint64_t max_length_{};
    uint64_t sum_length_{};
  )

  std::pair<uint64_t, uint64_t> get_allocs_(const decomp_val_t& pos_c) {
    assert(chunks_[pos_c.quo].get(pos_c.mod));

    const uint8_t* ptr = ptrs_[pos_c.quo].get();
    assert(ptr != nullptr);

    // -1 means the difference of the above bit_tools::set_bit
    const uint64_t num = chunks_[pos_c.quo].popcnt() - 1;
    const uint64_t offset = chunks_[pos_c.quo].popcnt(pos_c.mod);

    uint64_t front_alloc = 0, back_alloc = 0;

    for (uint64_t i = 0; i < num; ++i) {
      uint64_t len = 0;
      len += vbyte::decode(ptr, len);
      if (i < offset) {
        front_alloc += len;
      } else {
        back_alloc += len;
      }
      ptr += len;
    }

    return {front_alloc, back_alloc};
  }

  ustr_view get_slice_(const decomp_val_t& pos_c) const {
    if (!chunks_[pos_c.quo].get(pos_c.mod)) {
      // pos indicates a step node
      return {nullptr, 0};
    }

    const uint8_t* ptr = ptrs_[pos_c.quo].get();
    assert(ptr != nullptr);

    const uint64_t offset = chunks_[pos_c.quo].popcnt(pos_c.mod);

    // Proceeds the target position
    uint64_t len = 0;
    for (uint64_t i = 0; i < offset; ++i) {
      ptr += vbyte::decode(ptr, len);
      ptr += len;
    }

    uint64_t vsize = vbyte::decode(ptr, len);
    return {ptr, vsize + len};
  }

  void set_slice_(decomp_val_t pos_c, const ustr_view& new_slice) {
    assert(!chunks_[pos_c.quo].get(pos_c.mod));

    chunks_[pos_c.quo].set(pos_c.mod);
    const uint8_t* ptr = ptrs_[pos_c.quo].get();

    if (ptr == nullptr) {
      // First association in the group
      ptrs_[pos_c.quo] = std::make_unique<uint8_t[]>(new_slice.size());
      copy_bytes(ptrs_[pos_c.quo].get(), new_slice.data(), new_slice.size());
      return;
    }

    // Second and subsequent association in the group
    auto fr_alloc = get_allocs_(pos_c);
    auto new_unique =
      std::make_unique<uint8_t[]>(fr_alloc.first + new_slice.size() + fr_alloc.second);

    uint8_t* new_ptr = new_unique.get();

    // Copy the front allocation
    copy_bytes(new_ptr, ptr, fr_alloc.first);
    ptr += fr_alloc.first;
    new_ptr += fr_alloc.first;

    // Set new label
    copy_bytes(new_ptr, new_slice.data(), new_slice.size());
    new_ptr += new_slice.size();

    // Copy back
    copy_bytes(new_ptr, ptr, fr_alloc.second);
    ptrs_[pos_c.quo] = std::move(new_unique);
  }
};

} //ns - poplar

#endif //POPLAR_TRIE_LABEL_STORE_GM_HPP
