#ifndef POPLAR_TRIE_PLAIN_LABEL_STORE_HPP
#define POPLAR_TRIE_PLAIN_LABEL_STORE_HPP

#include <memory>
#include <vector>

#include "IntVector.hpp"
#include "basics.hpp"

namespace poplar {

template <typename Value>
class PlainLabelStore {
 public:
  using ValueType = Value;

 public:
  PlainLabelStore() = default;

  explicit PlainLabelStore(uint32_t capa_bits) : ptrs_(1ULL << capa_bits) {}

  ~PlainLabelStore() = default;

  std::pair<const Value*, uint64_t> compare(uint64_t pos, const ustr_view& key) const {
    assert(ptrs_[pos]);

    const uint8_t* ptr = ptrs_[pos].get();

    if (key.empty()) {
      return {reinterpret_cast<const Value*>(ptr), 0};
    }

    for (uint64_t i = 0; i < key.length(); ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    return {reinterpret_cast<const Value*>(ptr + key.length()), key.length()};
  }

  Value* associate(uint64_t pos, const ustr_view& key) {
    assert(!ptrs_[pos]);

    ++size_;

    uint64_t length = key.length();
    ptrs_[pos] = std::make_unique<uint8_t[]>(length + sizeof(ValueType));
    auto ptr = ptrs_[pos].get();
    copy_bytes(ptr, key.data(), length);

#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    auto ret = reinterpret_cast<Value*>(ptr + length);
    *ret = static_cast<Value>(0);

    return ret;
  }

  template <typename T>
  void expand(const T& pos_map) {
    std::vector<std::unique_ptr<uint8_t[]>> new_ptrs(capa_size() * 2);
    for (uint64_t i = 0; i < pos_map.size(); ++i) {
      if (pos_map[i] != UINT64_MAX) {
        new_ptrs[pos_map[i]] = std::move(ptrs_[i]);
      }
    }
    ptrs_ = std::move(new_ptrs);
  }

  uint64_t size() const {
    return size_;
  }
  uint64_t capa_size() const {
    return ptrs_.size();
  }

  void show_stat(std::ostream& os, int level = 0) const {
    std::string indent(level, '\t');
    os << indent << "stat:PlainLabelStore\n";
    os << indent << "\tsize:" << size() << "\n";
    os << indent << "\tcapa_size:" << capa_size() << "\n";
#ifdef POPLAR_ENABLE_EX_STATS
    os << indent << "\tmax_length:" << max_length_ << "\n";
    os << indent << "\tave_length:" << double(sum_length_) / size() << "\n";
#endif
  }

  PlainLabelStore(const PlainLabelStore&) = delete;
  PlainLabelStore& operator=(const PlainLabelStore&) = delete;

  PlainLabelStore(PlainLabelStore&&) noexcept = default;
  PlainLabelStore& operator=(PlainLabelStore&&) noexcept = default;

 private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_{};
  uint64_t size_{};
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_{};
  uint64_t sum_length_{};
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_HPP
