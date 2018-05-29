#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

constexpr int UPDATE_RUNS = 1;
constexpr int FIND_RUNS = 1;

template <class t_map>
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

  t_map map;

  // insertion
  {
    std::array<double, UPDATE_RUNS> times{};

    for (int i = 0; i < UPDATE_RUNS; ++i) {
      map = t_map{capa_bits};
      Stopwatch watch;
      for (const auto& key : keys) {
        *map.update(key) = 1;
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
    auto ptr = map.find(key);
    if (ptr != nullptr && *ptr == 1) {
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
        volatile auto ret = map.find(key);
      }
      times[i] = watch.micro_sec() / keys.size();
      ;
    }

    num_queries = keys.size();
    find_time = get_average(times);
  }

  std::cout << "name:" << short_realname<t_map>() << "\n";
  std::cout << "keys:" << num_keys << "\n";
  std::cout << "queries:" << num_queries << "\n";
  std::cout << "update_us_key:" << update_time << "\n";
  std::cout << "find_us_query:" << find_time << "\n";
  std::cout << "ok:" << ok << "\n";
  std::cout << "ng:" << ng << "\n";

  return 0;
}

template <uint64_t N = 0>
std::enable_if_t<N == NUM_MAPS, int> bench_for_id(int, const char*, const char*, uint32_t) {
  return 1;
}
template <uint64_t N = 0>
    std::enable_if_t < N<NUM_MAPS, int> bench_for_id(int id, const char* key_name,
                                                     const char* query_name, uint32_t capa_bits) {
  if (id - 1 == N) {
    using map_type = std::tuple_element_t<N, map_types<>>;
    return speed_test<map_type>(key_name, query_name, capa_bits);
  }
  return bench_for_id<N + 1>(id, key_name, query_name, capa_bits);
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <query> <capa>\n";
  os << "<type>   type ID of maps\n";
  maps_list_all("  ", os);
  os << "<key>    path of input keywords\n";
  os << "<query>  path of input queries (optional)\n";
  os << "<capa>   #bits of initial capacity (optional)\n";
  os.flush();
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  if (argc < 3 || 5 < argc) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  int map_id = std::stoi(argv[1]);
  if (map_id < 1 || NUM_MAPS < map_id) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  const char* key_name = argv[2];
  const char* query_name = 4 <= argc ? argv[3] : "-";

  uint32_t capa_bits = 5 <= argc ? static_cast<uint32_t>(std::stoi(argv[4])) : 0;

  return bench_for_id(map_id, key_name, query_name, capa_bits);
}
