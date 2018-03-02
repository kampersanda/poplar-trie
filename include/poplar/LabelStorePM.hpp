#ifndef POPLAR_TRIE_LABEL_STORE_PM_HPP
#define POPLAR_TRIE_LABEL_STORE_PM_HPP

#include <vector>
#include <memory>

#include "basics.hpp"
#include "IntVector.hpp"

namespace poplar {

template <typename t_value>
class LabelStorePM {
public:
  using value_type = t_value;

public:
  LabelStorePM() = default;

  explicit LabelStorePM(uint32_t capa_bits) : ptrs_(1ULL << capa_bits) {}

  ~LabelStorePM() = default;

  std::pair<const t_value*, uint64_t> compare(uint64_t pos, const ustr_view& key) const {
    assert(ptrs_[pos]);

    const uint8_t* ptr = ptrs_[pos].get();

    if (key.empty()) {
      return {reinterpret_cast<const t_value*>(ptr), 0};
    }

    for (uint64_t i = 0; i < key.length(); ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    return {reinterpret_cast<const t_value*>(ptr + key.length()), key.length()};
  }

  t_value* associate(uint64_t pos, const ustr_view& key) {
    assert(!ptrs_[pos]);

    ++size_;

    uint64_t length = key.length();
    ptrs_[pos] = std::make_unique<uint8_t[]>(length + sizeof(value_type));
    auto ptr = ptrs_[pos].get();
    copy_bytes(ptr, key.data(), length);

    POPLAR_EX_STATS(
      max_length_ = std::max(max_length_, length);
      sum_length_ += length;
    )

    auto ret = reinterpret_cast<t_value*>(ptr + length);
    *ret = static_cast<t_value>(0);

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

  void show_stat(std::ostream& os, std::string&& level = "") const {
    os << level << "stat:LabelStorePM\n";
    os << level << "\tsize:" << size() << "\n";
    os << level << "\tcapa_size:" << capa_size() << "\n";
    POPLAR_EX_STATS(
      os << level << "\tmax_length:" << max_length_ << "\n";
      os << level << "\tave_length:" << double(sum_length_) / size() << "\n";
    )
  }

  void swap(LabelStorePM& rhs) {
    std::swap(ptrs_, rhs.ptrs_);
    std::swap(size_, rhs.size_);
    POPLAR_EX_STATS(
      std::swap(max_length_, rhs.max_length_);
      std::swap(sum_length_, rhs.sum_length_);
    )
  }

  LabelStorePM(const LabelStorePM&) = delete;
  LabelStorePM& operator=(const LabelStorePM&) = delete;

  LabelStorePM(LabelStorePM&& rhs) noexcept : LabelStorePM() {
    this->swap(rhs);
  }
  LabelStorePM& operator=(LabelStorePM&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

private:
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_{};
  uint64_t size_{};

  POPLAR_EX_STATS(
    uint64_t max_length_{};
    uint64_t sum_length_{};
  )
};

} //ns - poplar

#endif //POPLAR_TRIE_LABEL_STORE_PM_HPP
