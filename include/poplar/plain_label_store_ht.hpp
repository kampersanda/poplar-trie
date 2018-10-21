#ifndef POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
#define POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP

#include <memory>
#include <vector>

#include "basics.hpp"
#include "compact_vector.hpp"

namespace poplar {

template <typename Value>
class plain_label_store_ht {
 public:
  using value_type = Value;

  static constexpr auto trie_type = trie_types::HASH_TRIE;

 public:
  plain_label_store_ht() = default;

  explicit plain_label_store_ht(uint32_t capa_bits) {
    chars_.reserve(1ULL << capa_bits);
    ptrs_.reserve(1ULL << capa_bits);
    ptrs_.emplace_back(0);
  }

  ~plain_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos + 1 < ptrs_.size());

    const uint8_t* ptr = chars_.data() + ptrs_[pos];
    uint64_t alloc = ptrs_[pos + 1] - ptrs_[pos];

    if (key.empty()) {
      assert(sizeof(value_type) == alloc);
      return {reinterpret_cast<const value_type*>(ptr), 0};
    }

    assert(sizeof(value_type) <= alloc);

    uint64_t length = alloc - sizeof(value_type);
    for (uint64_t i = 0; i < length; ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    if (key[length] != '\0') {
      return {nullptr, length};
    }

    return {reinterpret_cast<const value_type*>(ptr + length), length + 1};
  }

  value_type* append(char_range key) {
#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    uint64_t length = key.empty() ? 0 : key.length() - 1;
    std::copy(key.begin, key.begin + length, std::back_inserter(chars_));

    const size_t vpos = chars_.size();
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      chars_.emplace_back(0);
    }

    ptrs_.emplace_back(chars_.size());
    return reinterpret_cast<value_type*>(chars_.data() + vpos);
  }

  void append_dummy() {
    ptrs_.emplace_back(chars_.size());
  }

  uint64_t size() const {
    return ptrs_.size() - 1;
  }
  uint64_t capa_size() const {
    return ptrs_.capacity();
  }

  boost::property_tree::ptree make_ptree() const {
    boost::property_tree::ptree pt;
    pt.put("name", "plain_label_store_ht");
    pt.put("size", size());
    pt.put("capa_size", capa_size());
#ifdef POPLAR_ENABLE_EX_STATS
    pt.put("max_length", max_length_);
    pt.put("ave_length", double(sum_length_) / size());
#endif
    return pt;
  }

  plain_label_store_ht(const plain_label_store_ht&) = delete;
  plain_label_store_ht& operator=(const plain_label_store_ht&) = delete;

  plain_label_store_ht(plain_label_store_ht&&) noexcept = default;
  plain_label_store_ht& operator=(plain_label_store_ht&&) noexcept = default;

 private:
  std::vector<uint8_t> chars_;
  std::vector<uint64_t> ptrs_;
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
