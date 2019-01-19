#include <iostream>

#include "cmdline.h"
#include "common.hpp"

namespace {

using namespace poplar;

constexpr int UPDATE_RUNS = 3;
constexpr int FIND_RUNS = 10;

using value_type = int32_t;

template <class Map>
int measure(const std::string& key_name, const std::string& query_name, uint32_t capa_bits, uint64_t lambda) {
  Map map;
  uint64_t process_size = get_process_size();
  uint64_t num_keys = 0, num_queries = 0;
  uint64_t ok = 0, ng = 0;
  double update_time = 0.0, find_time = 0.0;

  {
    std::ifstream ifs{key_name};
    if (!ifs) {
      std::cerr << "error: failed to open " << key_name << std::endl;
      return 1;
    }

    map = Map{capa_bits, lambda};

    std::string key;
    key.reserve(1024);

    while (std::getline(ifs, key)) {
      *map.update(make_char_range(key)) = 1;
    }
    process_size = get_process_size() - process_size;
  }

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

  {
    std::array<double, UPDATE_RUNS> times{};

    for (int i = 0; i < UPDATE_RUNS; ++i) {
      map = Map{capa_bits, lambda};
      timer t;
      for (const auto& key : keys) {
        *map.update(make_char_range(key)) = 1;
      }
      times[i] = t.get<std::micro>() / keys.size();
    }

    num_keys = keys.size();
    update_time = get_average(times);
  }

  if (query_name != "-") {
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
      uint64_t _ok = 0, _ng = 0;

      timer t;
      for (const auto& key : keys) {
        auto ptr = map.find(make_char_range(key));
        if (ptr != nullptr and *ptr == 1) {
          ++_ok;
        } else {
          ++_ng;
        }
      }
      times[i] = t.get<std::micro>() / keys.size();

      if ((ok != _ok) or (ng != _ng)) {
        std::cerr << "critical error for search results" << std::endl;
        return 1;
      }
    }

    num_queries = keys.size();
    find_time = get_average(times);
  }

  std::ostream& out = std::cout;
  auto indent = get_indent(0);

  show_stat(out, indent, "map_name", short_realname<Map>());
  show_stat(out, indent, "key_name", key_name);
  show_stat(out, indent, "query_name", query_name);
  show_stat(out, indent, "init_capa_bits", capa_bits);

  show_stat(out, indent, "rss_bytes", process_size);
  show_stat(out, indent, "rss_MiB", process_size / (1024.0 * 1024.0));

  show_stat(out, indent, "num_keys", num_keys);
  show_stat(out, indent, "num_queries", num_queries);

  show_stat(out, indent, "update_us_key", update_time);
  show_stat(out, indent, "find_us_query", find_time);
  show_stat(out, indent, "update_runs", UPDATE_RUNS);
  show_stat(out, indent, "find_runs", FIND_RUNS);

  show_stat(out, indent, "ok", ok);
  show_stat(out, indent, "ng", ng);

  show_member(out, indent, "map");
  map.show_stats(out, 1);

  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
  p.add<std::string>("query_fn", 'q', "input file name of queries", false, "-");
  p.add<std::string>("map_type", 't', "plain_bt | compact_bt | plain_ht | compact_ht | rrr_ht", true);
  p.add<uint32_t>("chunk_size", 'c', "8 | 16 | 32 | 64 (for compact_bt and compact_ht)", false, 16);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.add<uint64_t>("lambda", 'l', "lambda", false, 32);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto query_fn = p.get<std::string>("query_fn");
  auto map_type = p.get<std::string>("map_type");
  auto chunk_size = p.get<uint32_t>("chunk_size");
  auto capa_bits = p.get<uint32_t>("capa_bits");
  auto lambda = p.get<uint64_t>("lambda");

  try {
    if (map_type == "plain_bt") {
      return measure<plain_bonsai_map<value_type>>(key_fn, query_fn, capa_bits, lambda);
    } else if (map_type == "compact_bt") {
      switch (chunk_size) {
        case 8:
          return measure<compact_bonsai_map<value_type, 8>>(key_fn, query_fn, capa_bits, lambda);
        case 16:
          return measure<compact_bonsai_map<value_type, 16>>(key_fn, query_fn, capa_bits, lambda);
        case 32:
          return measure<compact_bonsai_map<value_type, 32>>(key_fn, query_fn, capa_bits, lambda);
        case 64:
          return measure<compact_bonsai_map<value_type, 64>>(key_fn, query_fn, capa_bits, lambda);
        default:
          break;
      }
    } else if (map_type == "plain_ht") {
      return measure<plain_hash_map<value_type>>(key_fn, query_fn, capa_bits, lambda);
    } else if (map_type == "compact_ht") {
      switch (chunk_size) {
        case 8:
          return measure<compact_hash_map<value_type, 8>>(key_fn, query_fn, capa_bits, lambda);
        case 16:
          return measure<compact_hash_map<value_type, 16>>(key_fn, query_fn, capa_bits, lambda);
        case 32:
          return measure<compact_hash_map<value_type, 32>>(key_fn, query_fn, capa_bits, lambda);
        case 64:
          return measure<compact_hash_map<value_type, 64>>(key_fn, query_fn, capa_bits, lambda);
        default:
          break;
      }
    } else if (map_type == "rrr_ht") {
      return measure<rrr_hash_map<value_type>>(key_fn, query_fn, capa_bits, lambda);
    }
  } catch (const exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  std::cerr << p.usage() << std::endl;
  return 1;
}
