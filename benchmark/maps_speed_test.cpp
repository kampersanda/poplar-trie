#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

constexpr int UPDATE_RUNS = 1;
constexpr int FIND_RUNS = 1;

template <class Map>
int speed_test(const char* key_name, const char* query_name, uint32_t capa_bits) {
  boost::property_tree::ptree pt;
  pt.put("map_name", realname<Map>());
  pt.put("key_name", key_name);
  pt.put("query_name", query_name);
  pt.put("capa_bits", capa_bits);

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

  Map map;

  // insertion
  {
    std::array<double, UPDATE_RUNS> times{};

    for (int i = 0; i < UPDATE_RUNS; ++i) {
      map = Map{capa_bits};
      timer t;
      for (const auto& key : keys) {
        *map.update(make_char_range(key)) = 1;
      }
      times[i] = t.get<std::micro>() / keys.size();
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
    auto ptr = map.find(make_char_range(key));
    if (ptr != nullptr and *ptr == 1) {
      ++ok;
    } else {
      ++ng;
    }
  }

  // search
  {
    std::array<double, FIND_RUNS> times{};

    for (int i = 0; i < FIND_RUNS; ++i) {
      timer t;
      for (const auto& key : keys) {
        volatile auto ret = map.find(make_char_range(key));
      }
      times[i] = t.get<std::micro>() / keys.size();
    }

    num_queries = keys.size();
    find_time = get_average(times);
  }

  pt.put("num_keys", num_keys);
  pt.put("num_queries", num_queries);
  pt.put("update_us_key", update_time);
  pt.put("find_us_query", find_time);
  pt.put("ok", ok);
  pt.put("ng", ng);
  pt.add_child("map", map.make_ptree());

  boost::property_tree::write_json(std::cout, pt);

  return 0;
}

template <size_t N = 0>
int speed_test_with_id(int id, const char* key_name, const char* query_name, uint32_t capa_bits) {
  if constexpr (N >= NUM_MAPS) {
    return 1;
  } else {
    if (id - 1 == N) {
      return speed_test<std::tuple_element_t<N, map_types<>>>(key_name, query_name, capa_bits);
    }
    return speed_test_with_id<N + 1>(id, key_name, query_name, capa_bits);
  }
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <query> <capa>\n";
  os << "<type>   type ID of maps\n";
  list_all<map_types<>>("  ", os);
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
  if (map_id < 1 or NUM_MAPS < map_id) {
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
