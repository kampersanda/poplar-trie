#ifndef POPLAR_TRIE_COMPACT_CUCKOO_HASH_TRIE_HPP
#define POPLAR_TRIE_COMPACT_CUCKOO_HASH_TRIE_HPP

#include "BitChunk.hpp"
#include "BitVector.hpp"
#include "CompactHashTable.hpp"
#include "IntVector.hpp"
#include "bijective_hash.hpp"

namespace poplar {

template <uint32_t Factor = 80, uint64_t BucketSize = 8, uint64_t NumHashers = 2,
          typename Hasher = bijective_hash::SplitMix>
class CompactCuckooHashTrie {
 private:
  static_assert(0 < Factor and Factor < 100);
  static_assert(1 < BucketSize and is_power2(BucketSize));
  static_assert(1 < NumHashers and is_power2(NumHashers));

 public:
  static constexpr uint64_t NIL_ID = UINT64_MAX;
  static constexpr uint32_t MIN_CAPA_BITS = 16;
  static constexpr uint32_t BS_BITS = bit_tools::ceil_log2(BucketSize);
  static constexpr uint32_t NH_BITS = bit_tools::ceil_log2(NumHashers);

 public:
  CompactCuckooHashTrie() = default;

  CompactCuckooHashTrie(uint32_t capa_bits, uint32_t symb_bits) {
    capa_size_ = size_p2_t{std::max(MIN_CAPA_BITS, capa_bits)};  // M
    symb_size_ = size_p2_t{symb_bits};  // σ

    POPLAR_THROW_IF(capa_size_.bits() <= NH_BITS, "overflow");
    subtable_size_ = size_p2_t{capa_size_.bits() - NH_BITS};

    POPLAR_THROW_IF(subtable_size_.bits() <= BS_BITS, "overflow");
    subbucket_size_ = size_p2_t{subtable_size_.bits() - BS_BITS};

    max_size_ = static_cast<uint64_t>(capa_size_.size() * Factor / 100.0);

    for (uint32_t i = 0; i < NumHashers; ++i) {
      hashes_[i] = Hasher{capa_size_.bits() + symb_size_.bits(), i};
    }

    //  +1 for representing vacant slots
    table_ = IntVector{capa_size_.size(), symb_size_.bits() + BS_BITS + NH_BITS + 1};
  }

  ~CompactCuckooHashTrie() = default;

  uint64_t get_root() const {
    assert(size_ != 0);
    return 0;
  }

  void add_root() {
    assert(size_ == 0);
    size_ = 1;
    table_.set(0, 1U);  // set a non-vacant flag
  }

  uint64_t find_child(uint64_t node_id, uint64_t symb) const {
    assert(node_id < capa_size_.size());
    assert(symb < symb_size_.size());

    if (size_ == 0) {
      return NIL_ID;
    }

    // 1) Create the hash key
    const auto key = make_key_(node_id, symb);

    for (uint64_t k = 0; k < NumHashers; ++k) {
      // 2) Compute the target bucket ID and quotient for each subtable
      auto dv = decompose_(hashes_[k].hash(key));

      // of the target bucket
      uint64_t begin = k * subtable_size_.size() + dv.mod * BucketSize;
      uint64_t end = begin + BucketSize;

      // 3) Search the entry in the target bucket
      for (auto i = begin; i < end; ++i) {
        auto slot = table_[i];

        if ((slot & 1U) == 0U) {  // vacant?
          break;
        }

        if ((slot >> 1) == dv.quo) {  // registerd?
          return i;  // Found
        }
      }
    }

    return NIL_ID;
  }

  ac_res_type add_child(uint64_t& node_id, uint64_t symb) {
    assert(node_id < capa_size_.size());
    assert(symb < symb_size_.size());

    // 1) Create the hash key
    const auto key = make_key_(node_id, symb);

    uint64_t vacant_id = NIL_ID;
    uint64_t quotient = NIL_ID;

    for (uint32_t k = 0; k < NumHashers; ++k) {
      // 2) Compute the target bucket ID and quotient for each subtable
      auto dv = decompose_(hashes_[k].hash(key));

      // of the target bucket
      uint64_t begin = k * subtable_size_.size() + dv.mod * BucketSize;
      uint64_t end = begin + BucketSize;

      // 3) Search the entry in the target bucket
      for (auto i = begin; i < end; ++i) {
        auto slot = table_[i];

        if ((slot & 1U) == 0U) {  // vacant?
          vacant_id = i;
          quotient = dv.quo;
          break;
        }

        if ((slot >> 1) == dv.quo) {  // registerd?
          node_id = i;
          return ac_res_type::ALREADY_STORED;
        }
      }
    }

    if (vacant_id == NIL_ID or size_ == max_size_) {
      return ac_res_type::NEEDS_TO_EXPAND;
    }

    table_.set(vacant_id, (quotient << 1) | 1U);

    ++size_;
    node_id = vacant_id;

    return ac_res_type::SUCCESS;
  }

  std::pair<uint64_t, uint64_t> get_parent_and_symb(uint64_t node_id) const {
    assert(node_id < capa_size_.size());

    uint64_t slot = table_[node_id];

    if (node_id == 0 && (slot & 1U) == 0U) {  // root or vacant?
      return {NIL_ID, 0};
    }

    uint64_t subtable_id = node_id >> subtable_size_.bits();
    uint64_t bucket_id = (node_id / BucketSize) & subbucket_size_.mask();  // in the bucket

    uint64_t orig_key =
        hashes_[subtable_id].hash_inv((slot >> 1) << subbucket_size_.bits() | bucket_id);

    // Returns pair (parent, label)
    return std::make_pair(orig_key >> symb_size_.bits(), orig_key & symb_size_.mask());
  }

  uint64_t size() const {
    return size_;
  }

  uint64_t max_size() const {
    return max_size_;
  }

  uint64_t capa_size() const {
    return capa_size_.size();
  }

  uint32_t capa_bits() const {
    return capa_size_.bits();
  }

  uint64_t symb_size() const {
    return symb_size_.size();
  }

  uint32_t symb_bits() const {
    return symb_size_.bits();
  }

  void show_stat(std::ostream& os, int level = 0) const {
    std::string indent(level, '\t');
    os << indent << "stat:CompactCuckooHashTrie\n";
    os << indent << "\tfactor:" << Factor << "\n";
    os << indent << "\tsize:" << size() << "\n";
    os << indent << "\tcapa_size:" << capa_size() << "\n";
    os << indent << "\tcapa_bits:" << capa_bits() << "\n";
    os << indent << "\tsymb_size:" << symb_size() << "\n";
    os << indent << "\tsymb_bits:" << symb_bits() << "\n";
    os << indent << "\tsubtable_size:" << subtable_size_.size() << "\n";
    os << indent << "\tsubbucket_size:" << subbucket_size_.size() << "\n";
#ifdef POPLAR_ENABLE_EX_STATS
    os << indent << "\tnum_resize:" << num_resize_ << "\n";
#endif
  }

  CompactCuckooHashTrie(const CompactCuckooHashTrie&) = delete;
  CompactCuckooHashTrie& operator=(const CompactCuckooHashTrie&) = delete;

  CompactCuckooHashTrie(CompactCuckooHashTrie&&) noexcept = default;
  CompactCuckooHashTrie& operator=(CompactCuckooHashTrie&&) noexcept = default;

 private:
  Hasher hashes_[NumHashers] = {};
  IntVector table_{};
  uint64_t size_{};  // # of registered nodes
  uint64_t max_size_{};  // Factor% of the capacity
  size_p2_t capa_size_{};
  size_p2_t symb_size_{};
  size_p2_t subtable_size_{};  // # of slots per subtable
  size_p2_t subbucket_size_{};  //  # of buckets per subtable

#ifdef POPLAR_ENABLE_EX_STATS
  uint64_t num_resize_{};
#endif

  uint64_t make_key_(uint64_t node_id, uint64_t symb) const {
    return (node_id << symb_size_.bits()) | symb;
  }
  decomp_val_t decompose_(uint64_t x) const {
    return {x >> subbucket_size_.bits(), x & subbucket_size_.mask()};
  }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_COMPACT_CUCKOO_HASH_TRIE_HPP
