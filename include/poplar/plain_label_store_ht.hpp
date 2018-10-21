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
    ptrs_.reserve(1ULL << capa_bits);
  }

  ~plain_label_store_ht() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos < ptrs_.size());
    assert(ptrs_[pos]);

    const uint8_t* ptr = ptrs_[pos].get();

    if (key.empty()) {
      return {reinterpret_cast<const value_type*>(ptr), 0};
    }

    for (uint64_t i = 0; i < key.length(); ++i) {
      if (key[i] != ptr[i]) {
        return {nullptr, i};
      }
    }

    return {reinterpret_cast<const value_type*>(ptr + key.length()), key.length()};
  }

  value_type* associate(uint64_t pos, char_range key) {
    assert(pos == ptrs_.size());

    uint64_t length = key.length();
    auto new_uptr = std::make_unique<uint8_t[]>(length + sizeof(value_type));
    auto ptr = new_uptr.get();
    copy_bytes(ptr, key.begin, length);

#ifdef POPLAR_ENABLE_EX_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    auto ret = reinterpret_cast<value_type*>(ptr + length);
    *ret = static_cast<value_type>(0);

    ptrs_.emplace_back(std::move(new_uptr));

    return ret;
  }

  void dummy_associate(uint64_t pos) {
    assert(pos == ptrs_.size());
    ptrs_.emplace_back(nullptr);
  }

  uint64_t size() const {
    return ptrs_.size();
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
  std::vector<std::unique_ptr<uint8_t[]>> ptrs_;
#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_LABEL_STORE_HT_HPP
