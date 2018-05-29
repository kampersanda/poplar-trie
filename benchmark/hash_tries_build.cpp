#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

template <class Trie>
int build(const char* key_name, uint32_t capa_bits) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    return 1;
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  auto trie = Trie{capa_bits, 8};

  try {
    std::string key;
    key.reserve(1024);

    Stopwatch watch;
    while (std::getline(ifs, key)) {
      auto node_id = trie.get_root();
      for (auto c : key) {
        auto ret = trie.add_child(node_id, static_cast<uint8_t>(c));
        if (ret == ac_res_type::NEEDS_TO_EXPAND) {
          std::cerr << "error\n";
          return 1;
        }
      }
      ++num_keys;
    }
    elapsed_sec = watch.sec();
  } catch (const Exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  std::cout << "name:" << short_realname<Trie>() << "\n";
  std::cout << "keys:" << num_keys << "\n";
  std::cout << "elapsed_sec:" << elapsed_sec << "\n";
  trie.show_stat(std::cout);

  return 0;
}

template <size_t N = 0>
int build_with_id(int id, const char* key_name, uint32_t capa_bits) {
  if constexpr (N >= NUM_HASH_TRIES) {
    return 1;
  } else {
    if (id - 1 == N) {
      return build<std::tuple_element_t<N, HashTrieTypes>>(key_name, capa_bits);
    }
    return build_with_id<N + 1>(id, key_name, capa_bits);
  }
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <capa>\n";
  os << "<type>  type ID of hash tries as follows:\n";
  list_all<HashTrieTypes>("  ", os);
  os << "<key>   path of input keywords\n";
  os << "<capa>  #bits of initial capacity (optional)\n";
  os.flush();
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  if (argc < 3 or 4 < argc) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  int map_id = std::stoi(argv[1]);
  if (map_id < 1 or NUM_HASH_TRIES < map_id) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  const char* key_name = argv[2];

  uint32_t capa_bits = 0;
  if (3 < argc) {
    capa_bits = static_cast<uint32_t>(std::stoi(argv[3]));
  }

  return build_with_id(map_id, key_name, capa_bits);
}