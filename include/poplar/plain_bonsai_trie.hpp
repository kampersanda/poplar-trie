/**
 * MIT License
 *
 * Copyright (c) 2018–2019 Shunsuke Kanda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef POPLAR_TRIE_PLAIN_BONSAI_TRIE_HPP
#define POPLAR_TRIE_PLAIN_BONSAI_TRIE_HPP

#include "bit_tools.hpp"
#include "bit_vector.hpp"
#include "compact_vector.hpp"
#include "hash.hpp"

namespace poplar {

template <uint32_t MaxFactor = 90, typename Hasher = hash::vigna_hasher>
class plain_bonsai_trie {
  private:
    static_assert(0 < MaxFactor and MaxFactor < 100);

  public:
    static constexpr uint64_t nil_id = UINT64_MAX;
    static constexpr uint32_t min_capa_bits = 16;

    static constexpr auto trie_type_id = trie_type_ids::BONSAI_TRIE;

  public:
    plain_bonsai_trie() = default;

    plain_bonsai_trie(uint32_t capa_bits, uint32_t symb_bits) {
        capa_size_ = size_p2{std::max(min_capa_bits, capa_bits)};
        symb_size_ = size_p2{symb_bits};
        max_size_ = static_cast<uint64_t>(capa_size_.size() * MaxFactor / 100.0);
        table_ = compact_vector{capa_size_.size(), capa_size_.bits() + symb_size_.bits()};
    }

    ~plain_bonsai_trie() = default;

    uint64_t get_root() const {
        assert(size_ != 0);
        return 1;
    }

    void add_root() {
        assert(size_ == 0);
        size_ = 1;
    }

    uint64_t find_child(uint64_t node_id, uint64_t symb) const {
        assert(node_id < capa_size_.size());
        assert(symb < symb_size_.size());

        if (size_ == 0) {
            return nil_id;
        }

        uint64_t key = make_key_(node_id, symb);
        assert(key != 0);

        for (uint64_t i = Hasher::hash(key) & capa_size_.mask();; i = right_(i)) {
            if (i == 0) {
                // table_[0] is always empty so that table_[i] = 0 indicates to be empty.
                continue;
            }
            if (i == get_root()) {
                continue;
            }
            if (table_[i] == 0) {
                // encounter an empty slot
                return nil_id;
            }
            if (table_[i] == key) {
                return i;
            }
        }
    }

    bool add_child(uint64_t& node_id, uint64_t symb) {
        assert(node_id < capa_size_.size());
        assert(symb < symb_size_.size());

        uint64_t key = make_key_(node_id, symb);
        assert(key != 0);

        for (uint64_t i = Hasher::hash(key) & capa_size_.mask();; i = right_(i)) {
            if (i == 0) {
                // table_[0] is always empty so that any table_[i] = 0 indicates to be empty.
                continue;
            }

            if (i == get_root()) {
                continue;
            }

            if (table_[i] == 0) {
                // this slot is empty
                if (size_ == max_size_) {
                    return false;  // needs to expand
                }

                table_.set(i, key);

                ++size_;
                node_id = i;

                return true;
            }

            if (table_[i] == key) {
                node_id = i;
                return false;  // already stored
            }
        }
    }

    std::pair<uint64_t, uint64_t> get_parent_and_symb(uint64_t node_id) const {
        assert(node_id < capa_size_.size());

        uint64_t key = table_[node_id];
        if (key == 0) {
            // root or not exist
            return {nil_id, 0};
        }
        // Returns pair (parent, label)
        return std::make_pair(key >> symb_size_.bits(), key & symb_size_.mask());
    };

    class node_map {
      public:
        node_map() = default;

        node_map(compact_vector&& map, bit_vector&& done_flags)
            : map_{std::move(map)}, done_flags_{std::move(done_flags)} {}

        ~node_map() = default;

        uint64_t operator[](uint64_t i) const {
            return done_flags_[i] ? map_[i] : UINT64_MAX;
        }

        uint64_t size() const {
            return map_.size();
        }

        node_map(const node_map&) = delete;
        node_map& operator=(const node_map&) = delete;

        node_map(node_map&& rhs) noexcept = default;
        node_map& operator=(node_map&& rhs) noexcept = default;

      private:
        compact_vector map_;
        bit_vector done_flags_;
    };

    bool needs_to_expand() const {
        return max_size() <= size();
    }

    node_map expand() {
        plain_bonsai_trie new_ht{capa_bits() + 1, symb_size_.bits()};
        new_ht.add_root();

#ifdef POPLAR_EXTRA_STATS
        new_ht.num_resize_ = num_resize_ + 1;
#endif

        bit_vector done_flags(capa_size());
        done_flags.set(get_root());

        table_.set(get_root(), new_ht.get_root());

        std::vector<std::pair<uint64_t, uint64_t>> path;
        path.reserve(256);

        // 0 is empty, 1 is root
        for (uint64_t i = 2; i < table_.size(); ++i) {
            if (done_flags[i] || table_[i] == 0) {
                // skip already processed or empty elements
                continue;
            }

            path.clear();
            uint64_t node_id = i;

            do {
                auto [parent, label] = get_parent_and_symb(node_id);
                assert(parent != nil_id);
                path.emplace_back(std::make_pair(node_id, label));
                node_id = parent;
            } while (!done_flags[node_id]);

            uint64_t new_node_id = table_[node_id];

            for (auto rit = std::rbegin(path); rit != std::rend(path); ++rit) {
                new_ht.add_child(new_node_id, rit->second);
                table_.set(rit->first, new_node_id);
                done_flags.set(rit->first);
            }
        }

        node_map node_map{std::move(table_), std::move(done_flags)};
        std::swap(*this, new_ht);

        return node_map;
    }

    // # of registerd nodes
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
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize() const {
        return num_resize_;
    }
#endif
    uint64_t alloc_bytes() const {
        return table_.alloc_bytes();
    }

    void show_stats(std::ostream& os, int n = 0) const {
        auto indent = get_indent(n);
        show_stat(os, indent, "name", "plain_bonsai_trie");
        show_stat(os, indent, "factor", double(size()) / capa_size() * 100);
        show_stat(os, indent, "max_factor", MaxFactor);
        show_stat(os, indent, "size", size());
        show_stat(os, indent, "alloc_bytes", alloc_bytes());
        show_stat(os, indent, "capa_bits", capa_bits());
        show_stat(os, indent, "symb_bits", symb_bits());
#ifdef POPLAR_EXTRA_STATS
        show_stat(os, indent, "num_resize", num_resize_);
#endif
    }

    plain_bonsai_trie(const plain_bonsai_trie&) = delete;
    plain_bonsai_trie& operator=(const plain_bonsai_trie&) = delete;

    plain_bonsai_trie(plain_bonsai_trie&&) noexcept = default;
    plain_bonsai_trie& operator=(plain_bonsai_trie&&) noexcept = default;

  private:
    compact_vector table_;
    uint64_t size_ = 0;  // # of registered nodes
    uint64_t max_size_ = 0;  // MaxFactor% of the capacity
    size_p2 capa_size_;
    size_p2 symb_size_;
#ifdef POPLAR_EXTRA_STATS
    uint64_t num_resize_ = 0;
#endif

    uint64_t make_key_(uint64_t node_id, uint64_t symb) const {
        return (node_id << symb_size_.bits()) | symb;
    }
    uint64_t right_(uint64_t slot_id) const {
        return (slot_id + 1) & capa_size_.mask();
    }
};

}  // namespace poplar

#endif  // POPLAR_TRIE_PLAIN_BONSAI_TRIE_HPP
