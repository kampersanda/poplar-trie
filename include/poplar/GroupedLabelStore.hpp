#ifndef POPLAR_TRIE_GROUPED_LABEL_STORE_HPP
#define POPLAR_TRIE_GROUPED_LABEL_STORE_HPP

#include <memory>
#include <vector>

#include "BitChunk.hpp"
#include "vbyte.hpp"

namespace poplar {

template <typename Value, typename Chunk>
class GroupedLabelStore {
 public:
  using ThisType = GroupedLabelStore<Value, Chunk>;
  using ValueType = Value;
  using ChunkType = Chunk;

  static constexpr uint64_t CHUNK_SIZE = ChunkType::LEN;

 public:
  GroupedLabelStore() = default;

  explicit GroupedLabelStore(uint32_t capa_bits)
      : ptrs_((1ULL << capa_bits) / CHUNK_SIZE), chunks_(ptrs_.size()) {}

  ~GroupedLabelStore() = default;

  std::pair<const Value*, uint64_t> compare(uint64_t pos, const ustr_view& key) const {
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
      return {reinterpret_cast<const Value*>(ptr), 0};
    }

    uint64_t length = alloc - sizeof(Value);
    for (uint64_t i = 0; i < length; ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    if (key[length] != '\0') {
      return {nullptr, length};
    }

    // +1 considers the terminator '\0'
    return {reinterpret_cast<const Value*>(ptr + length), length + 1};
  };

  Value* associate(uint64_t pos, const ustr_view& key) {
    const decomp_val_t pos_c = decompose_value<CHUNK_SIZE>(pos);

    assert(!chunks_[pos_c.quo].get(pos_c.mod));
    chunks_[pos_c.quo].set(pos_c.mod);

    ++size_;

#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max<uint64_t>(max_length_, key.length());
    sum_length_ += key.length();
#endif

    if (!ptrs_[pos_c.quo]) {
      // First association in the group
      uint64_t length = key.empty() ? 0 : key.length() - 1;
      uint64_t new_alloc = vbyte::size(length + sizeof(ValueType)) + length + sizeof(ValueType);

      ptrs_[pos_c.quo] = std::make_unique<uint8_t[]>(new_alloc);
      uint8_t* ptr = ptrs_[pos_c.quo].get();

      ptr += vbyte::encode(ptr, length + sizeof(ValueType));
      copy_bytes(ptr, key.data(), length);

      auto ret_ptr = reinterpret_cast<ValueType*>(ptr + length);
      *ret_ptr = static_cast<ValueType>(0);

      return ret_ptr;
    }

    // Second and subsequent association in the group
    auto fr_alloc = get_allocs_(pos_c);

    const uint64_t len = key.empty() ? 0 : key.length() - 1;
    const uint64_t new_alloc = vbyte::size(len + sizeof(ValueType)) + len + sizeof(ValueType);

    auto new_unique = std::make_unique<uint8_t[]>(fr_alloc.first + new_alloc + fr_alloc.second);

    // Get raw pointers
    const uint8_t* orig_ptr = ptrs_[pos_c.quo].get();
    uint8_t* new_ptr = new_unique.get();

    // Copy the front allocation
    copy_bytes(new_ptr, orig_ptr, fr_alloc.first);
    orig_ptr += fr_alloc.first;
    new_ptr += fr_alloc.first;

    // Set new allocation
    new_ptr += vbyte::encode(new_ptr, len + sizeof(ValueType));
    copy_bytes(new_ptr, key.data(), len);
    new_ptr += len;
    *reinterpret_cast<ValueType*>(new_ptr) = static_cast<ValueType>(0);

    // Copy the back allocation
    copy_bytes(new_ptr + sizeof(ValueType), orig_ptr, fr_alloc.second);

    // Overwrite
    ptrs_[pos_c.quo] = std::move(new_unique);

    return reinterpret_cast<ValueType*>(new_ptr);
  }

  template <typename T>
  void expand(const T& pos_map) {
    ThisType new_ls(bit_tools::get_num_bits(capa_size()));

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
#ifdef POPLAR_ENABLE_EX_STATS
    new_ls.max_length_ = max_length_;
    new_ls.sum_length_ = sum_length_;
#endif

    *this = std::move(new_ls);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t capa_size() const {
    return ptrs_.size() * CHUNK_SIZE;
  }

  void show_stat(std::ostream& os, int level = 0) const {
    std::string indent(level, '\t');
    os << indent << "stat:GroupedLabelStore\n";
    os << indent << "\tchunk_size:" << CHUNK_SIZE << "\n";
    os << indent << "\tsize:" << size() << "\n";
    os << indent << "\tcapa_size:" << capa_size() << "\n";
#ifdef POPLAR_ENABLE_EX_STATS
    os << indent << "\tmax_length:" << max_length_ << "\n";
    os << indent << "\tave_length:" << double(sum_length_) / size() << "\n";
#endif
  }

  GroupedLabelStore(const GroupedLabelStore&) = delete;
  GroupedLabelStore& operator=(const GroupedLabelStore&) = delete;

  GroupedLabelStore(GroupedLabelStore&&) noexcept = default;
  GroupedLabelStore& operator=(GroupedLabelStore&&) noexcept = default;

 private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_{};
  std::vector<ChunkType> chunks_{};
  uint64_t size_{};
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_{};
  uint64_t sum_length_{};
#endif

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

}  // namespace poplar

#endif  // POPLAR_TRIE_GROUPED_LABEL_STORE_HPP
