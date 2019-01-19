#include <iostream>

#include "cmdline.h"
#include "common.hpp"

namespace {

using namespace poplar;

template <class Map>
void build(const std::string& key_name, uint32_t capa_bits, uint64_t lambda) {
  uint64_t process_size = get_process_size();

  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    exit(1);
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  Map map{capa_bits, lambda};

  try {
    std::string key;
    key.reserve(1024);

    timer t;
    while (std::getline(ifs, key)) {
      map.update(make_char_range(key));
      ++num_keys;
    }
    elapsed_sec = t.get<>();
    process_size = get_process_size() - process_size;
  } catch (const exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  std::ostream& out = std::cout;
  auto indent = get_indent(0);

  show_stat(out, indent, "map_name", short_realname<Map>());
  show_stat(out, indent, "key_name", key_name);
  show_stat(out, indent, "init_capa_bits", capa_bits);
  show_stat(out, indent, "num_keys", num_keys);
  show_stat(out, indent, "elapsed_sec", elapsed_sec);
  show_stat(out, indent, "rss_bytes", process_size);
  show_stat(out, indent, "rss_MiB", process_size / (1024.0 * 1024.0));

  show_member(out, indent, "map");
  map.show_stats(out, 1);

  out << "-----" << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.add<uint64_t>("lambda", 'l', "lambda", false, 32);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto capa_bits = p.get<uint32_t>("capa_bits");
  auto lambda = p.get<uint64_t>("lambda");

  using ls_type = compact_label_store_bt<int, 16>;

  using map_80_3_type = map<compact_bonsai_trie<80, 3>, ls_type>;
  using map_85_3_type = map<compact_bonsai_trie<85, 3>, ls_type>;
  using map_90_3_type = map<compact_bonsai_trie<90, 3>, ls_type>;
  using map_95_3_type = map<compact_bonsai_trie<95, 3>, ls_type>;

  using map_80_4_type = map<compact_bonsai_trie<80, 4>, ls_type>;
  using map_85_4_type = map<compact_bonsai_trie<85, 4>, ls_type>;
  using map_90_4_type = map<compact_bonsai_trie<90, 4>, ls_type>;
  using map_95_4_type = map<compact_bonsai_trie<95, 4>, ls_type>;

  using map_80_5_type = map<compact_bonsai_trie<80, 5>, ls_type>;
  using map_85_5_type = map<compact_bonsai_trie<85, 5>, ls_type>;
  using map_90_5_type = map<compact_bonsai_trie<90, 5>, ls_type>;
  using map_95_5_type = map<compact_bonsai_trie<95, 5>, ls_type>;

  build<map_80_3_type>(key_fn, capa_bits, lambda);
  build<map_85_3_type>(key_fn, capa_bits, lambda);
  build<map_90_3_type>(key_fn, capa_bits, lambda);
  build<map_95_3_type>(key_fn, capa_bits, lambda);

  build<map_80_4_type>(key_fn, capa_bits, lambda);
  build<map_85_4_type>(key_fn, capa_bits, lambda);
  build<map_90_4_type>(key_fn, capa_bits, lambda);
  build<map_95_4_type>(key_fn, capa_bits, lambda);

  build<map_80_5_type>(key_fn, capa_bits, lambda);
  build<map_85_5_type>(key_fn, capa_bits, lambda);
  build<map_90_5_type>(key_fn, capa_bits, lambda);
  build<map_95_5_type>(key_fn, capa_bits, lambda);

  return 0;
}
