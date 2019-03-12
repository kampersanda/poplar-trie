#ifndef POPLAR_TRIE_PLAIN_FKHASH_NLM_HPP
#define POPLAR_TRIE_PLAIN_FKHASH_NLM_HPP

#include <vector>

#include "basics.hpp"
#include "exception.hpp"

namespace poplar {

template <typename Value>
class plain_fkhash_nlm {
 public:
  using value_type = Value;

  static constexpr auto trie_type_id = trie_type_ids::FKHASH_TRIE;

 public:
  plain_fkhash_nlm() = default;

  explicit plain_fkhash_nlm(uint32_t capa_bits) {
    ptrs_.reserve(1ULL << capa_bits);
  }

  ~plain_fkhash_nlm() = default;

  std::pair<const value_type*, uint64_t> compare(uint64_t pos, char_range key) const {
    assert(pos < ptrs_.size());
    assert(ptrs_[pos] < chars_.size());

    const uint8_t* char_ptr = chars_.data() + ptrs_[pos];

    if (key.empty()) {
      return {reinterpret_cast<const value_type*>(char_ptr), 0};
    }

    for (uint64_t i = 0; i < key.length(); ++i) {
      if (key[i] != char_ptr[i]) {
        return {nullptr, i};
      }
    }

    return {reinterpret_cast<const value_type*>(char_ptr + key.length()), key.length()};
  }

  value_type* append(char_range key) {
    ptrs_.push_back(chars_.size());
    std::copy(key.begin, key.end, std::back_inserter(chars_));
    for (size_t i = 0; i < sizeof(value_type); ++i) {
      chars_.emplace_back('\0');
    }

#ifdef POPLAR_EXTRA_STATS
    max_length_ = std::max(max_length_, length);
    sum_length_ += length;
#endif

    return reinterpret_cast<value_type*>(chars_.data() + chars_.size() - sizeof(value_type));
  }

  void append_dummy() {
    ptrs_.push_back(UINT64_MAX);
  }

  uint64_t size() const {
    return ptrs_.size();
  }

  void show_stats(std::ostream& os, int n = 0) const {
    auto indent = get_indent(n);
    show_stat(os, indent, "name", "plain_fkhash_nlm");
    show_stat(os, indent, "size", size());
#ifdef POPLAR_EXTRA_STATS
    show_stat(os, indent, "max_length", max_length_);
    show_stat(os, indent, "ave_length", double(sum_length_) / size());
#endif
  }

  plain_fkhash_nlm(const plain_fkhash_nlm&) = delete;
  plain_fkhash_nlm& operator=(const plain_fkhash_nlm&) = delete;

  plain_fkhash_nlm(plain_fkhash_nlm&&) noexcept = default;
  plain_fkhash_nlm& operator=(plain_fkhash_nlm&&) noexcept = default;

 private:
  std::vector<uint8_t> chars_;
  std::vector<uint64_t> ptrs_;
#ifdef POPLAR_EXTRA_STATS
  uint64_t max_length_ = 0;
  uint64_t sum_length_ = 0;
#endif
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_FKHASH_NLM_HPP
