#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

constexpr int UPDATE_RUNS = 1;
constexpr int FIND_RUNS = 1;

template <class Trie>
int speed_test(const char* key_name, const char* query_name, uint32_t capa_bits) {
  std::vector<std::string> keys;
  {
    std::ifstream ifs{key_name};
    if (!ifs) {
      std::cerr << "Error: failed to open " << key_name << std::endl;
      return 1;
    }
    for (std::string line; std::getline(ifs, line);) {
      keys.push_back(line);
    }
  }

  uint64_t num_keys = 0, num_queries = 0;
  uint64_t ok = 0, ng = 0;
  double update_time = 0.0, find_time = 0.0;

  Trie trie;

  // insertion
  {
    std::array<double, UPDATE_RUNS> times{};

    for (int i = 0; i < UPDATE_RUNS; ++i) {
      trie = Trie{capa_bits, 8};
      Stopwatch watch;

      for (const auto& key : keys) {
        auto node_id = trie.get_root();
        for (auto c : key) {
          auto ret = trie.add_child(node_id, static_cast<uint8_t>(c));
          if (ret == ac_res_type::NEEDS_TO_EXPAND) {
            std::cerr << "error\n";
            return 1;
          }
        }
      }

      times[i] = watch.micro_sec() / keys.size();
    }

    num_keys = keys.size();
    update_time = get_average(times);
  }

  if (std::strcmp(query_name, "-") != 0) {
    std::ifstream ifs{query_name};
    if (!ifs) {
      std::cerr << "Error: failed to open " << query_name << std::endl;
      return 1;
    }
    keys.clear();
    for (std::string line; std::getline(ifs, line);) {
      keys.push_back(line);
    }
  }

  // warming up
  for (const auto& key : keys) {
    auto node_id = trie.get_root();

    for (auto c : key) {
      node_id = trie.find_child(node_id, static_cast<uint8_t>(c));
      if (node_id == Trie::NIL_ID) {
        break;
      }
    }

    if (node_id != Trie::NIL_ID) {
      ++ok;
    } else {
      ++ng;
    }
  }

  // search
  {
    std::array<double, FIND_RUNS> times{};

    for (int i = 0; i < FIND_RUNS; ++i) {
      Stopwatch watch;
      for (const auto& key : keys) {
        auto node_id = trie.get_root();
        for (auto c : key) {
          node_id = trie.find_child(node_id, static_cast<uint8_t>(c));
          if (node_id == Trie::NIL_ID) {
            break;
          }
        }
      }
      times[i] = watch.micro_sec() / keys.size();
    }

    num_queries = keys.size();
    find_time = get_average(times);
  }

  std::cout << "name:" << short_realname<Trie>() << "\n";
  std::cout << "keys:" << num_keys << "\n";
  std::cout << "queries:" << num_queries << "\n";
  std::cout << "update_us_key:" << update_time << "\n";
  std::cout << "find_us_query:" << find_time << "\n";
  std::cout << "ok:" << ok << "\n";
  std::cout << "ng:" << ng << "\n";

  return 0;
}

template <size_t N = 0>
int speed_test_with_id(int id, const char* key_name, const char* query_name, uint32_t capa_bits) {
  if constexpr (N >= NUM_HASH_TRIES) {
    return 1;
  } else {
    if (id - 1 == N) {
      return speed_test<std::tuple_element_t<N, HashTrieTypes>>(key_name, query_name, capa_bits);
    }
    return speed_test_with_id<N + 1>(id, key_name, query_name, capa_bits);
  }
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <query> <capa>\n";
  os << "<type>   type ID of hash tries as follows:\n";
  list_all<HashTrieTypes>("  ", os);
  os << "<key>    path of input keywords\n";
  os << "<query>  path of input queries (optional)\n";
  os << "<capa>   #bits of initial capacity (optional)\n";
  os.flush();
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  if (argc < 3 or 5 < argc) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  int map_id = std::stoi(argv[1]);
  if (map_id < 1 or NUM_HASH_TRIES < map_id) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  const char* key_name = argv[2];
  const char* query_name = 4 <= argc ? argv[3] : "-";

  uint32_t capa_bits = 0;
  if (4 < argc) {
    capa_bits = static_cast<uint32_t>(std::stoi(argv[4]));
  }

  return speed_test_with_id(map_id, key_name, query_name, capa_bits);
}
