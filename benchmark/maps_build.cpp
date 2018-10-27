#include <iostream>

#include "cmdline.h"
#include "common.hpp"
#include "hash_trie_map.hpp"

namespace {

using namespace poplar;

// To identify poplar::map with hash_trie_map
using value1_type = int32_t;
using value2_type = uint32_t;

template <class Map>
int build(const std::string& key_name, uint32_t capa_bits) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    return 1;
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  Map map{capa_bits};

  try {
    std::string key;
    key.reserve(1024);

    timer t;
    while (std::getline(ifs, key)) {
      if constexpr (std::is_same_v<typename Map::value_type, value1_type>) {
        // poplar::map
        *map.update(make_char_range(key)) = 1;
      }
      if constexpr (std::is_same_v<typename Map::value_type, value2_type>) {
        // poplar::hash_trie_map
        map.update(key, 1);
      }
      ++num_keys;
    }
    elapsed_sec = t.get<>();
  } catch (const exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  std::ostream& out = std::cout;
  auto indent = get_indent(0);

  show_stat(out, indent, "map_name", realname<Map>());
  show_stat(out, indent, "key_name", key_name);
  show_stat(out, indent, "init_capa_bits", capa_bits);
  show_stat(out, indent, "num_keys", num_keys);
  show_stat(out, indent, "elapsed_sec", elapsed_sec);

  show_member(out, indent, "map");
  map.show_stats(out, 1);

  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
  p.add<std::string>("map_type", 't', "plain_bonsai/compact_bonsai/plain_hash/compact_hash/rrr_hash", true);
  p.add<uint32_t>("chunk_size", 'c', "8/16/32/64 (for compact_bonsai and compact_hash)", false, 16);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.add<bool>("ht_map", 'h', "use hash_trie_map", false, false);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto map_type = p.get<std::string>("map_type");
  auto chunk_size = p.get<uint32_t>("chunk_size");
  auto capa_bits = p.get<uint32_t>("capa_bits");
  auto ht_map = p.get<bool>("ht_map");

  constexpr uint64_t lambda = 16;

  if (map_type == "plain_bonsai") {
    if (!ht_map) {
      return build<plain_bonsai_map<value1_type, lambda>>(key_fn, capa_bits);
    } else {
      using ht_type = hash_trie_map<plain_bonsai_trie<>, value2_type>;
      return build<ht_type>(key_fn, capa_bits);
    }
  } else if (map_type == "compact_bonsai") {
    if (!ht_map) {
      switch (chunk_size) {
        case 8:
          return build<compact_bonsai_map<value1_type, 8, lambda>>(key_fn, capa_bits);
        case 16:
          return build<compact_bonsai_map<value1_type, 16, lambda>>(key_fn, capa_bits);
        case 32:
          return build<compact_bonsai_map<value1_type, 32, lambda>>(key_fn, capa_bits);
        case 64:
          return build<compact_bonsai_map<value1_type, 64, lambda>>(key_fn, capa_bits);
        default:
          break;
      }
    } else {
      using ht_type = hash_trie_map<compact_bonsai_trie<>, value2_type>;
      return build<ht_type>(key_fn, capa_bits);
    }
  } else if (map_type == "plain_hash") {
    if (!ht_map) {
      return build<plain_hash_map<value1_type, lambda>>(key_fn, capa_bits);
    } else {
      using ht_type = hash_trie_map<plain_hash_trie<>, value2_type>;
      return build<ht_type>(key_fn, capa_bits);
    }
  } else if (map_type == "compact_hash") {
    if (!ht_map) {
      switch (chunk_size) {
        case 8:
          return build<compact_hash_map<value1_type, 8, lambda>>(key_fn, capa_bits);
        case 16:
          return build<compact_hash_map<value1_type, 16, lambda>>(key_fn, capa_bits);
        case 32:
          return build<compact_hash_map<value1_type, 32, lambda>>(key_fn, capa_bits);
        case 64:
          return build<compact_hash_map<value1_type, 64, lambda>>(key_fn, capa_bits);
        default:
          break;
      }
    } else {
      using ht_type = hash_trie_map<compact_hash_trie<>, value2_type>;
      return build<ht_type>(key_fn, capa_bits);
    }
  } else if (map_type == "rrr_hash") {
    if (!ht_map) {
      return build<rrr_hash_map<value1_type, lambda>>(key_fn, capa_bits);
    }
  }

  std::cerr << p.usage() << std::endl;
  return 1;
}