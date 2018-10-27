#include <iostream>

#include "benchmark_common.hpp"
#include "cmdline.h"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

constexpr int UPDATE_RUNS = 3;
constexpr int FIND_RUNS = 10;

template <class Map>
int measure(const std::string& key_name, const std::string& query_name, uint32_t capa_bits) {
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
      timer t;
      for (const auto& key : keys) {
        volatile auto ret = map.find(make_char_range(key));
      }
      times[i] = t.get<std::micro>() / keys.size();
    }

    num_queries = keys.size();
    find_time = get_average(times);
  }

  std::ostream& out = std::cout;
  auto indent = get_indent(0);

  show_stat(out, indent, "map_name", realname<Map>());
  show_stat(out, indent, "key_name", key_name);
  show_stat(out, indent, "query_name", query_name);
  show_stat(out, indent, "init_capa_bits", capa_bits);

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
  p.add<std::string>("map_type", 't', "plain_bonsai/compact_bonsai/plain_hash/compact_hash/rrr_hash", true);
  p.add<uint32_t>("chunk_size", 'c', "8/16/32/64 (for compact_bonsai and compact_hash)", false, 16);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto query_fn = p.get<std::string>("query_fn");
  auto map_type = p.get<std::string>("map_type");
  auto chunk_size = p.get<uint32_t>("chunk_size");
  auto capa_bits = p.get<uint32_t>("capa_bits");

  using value_type = int;
  constexpr uint64_t lambda = 16;

  if (map_type == "plain_bonsai") {
    return measure<plain_bonsai_map<value_type, lambda>>(key_fn, query_fn, capa_bits);
  } else if (map_type == "compact_bonsai") {
    switch (chunk_size) {
      case 8:
        return measure<compact_bonsai_map<value_type, 8, lambda>>(key_fn, query_fn, capa_bits);
      case 16:
        return measure<compact_bonsai_map<value_type, 16, lambda>>(key_fn, query_fn, capa_bits);
      case 32:
        return measure<compact_bonsai_map<value_type, 32, lambda>>(key_fn, query_fn, capa_bits);
      case 64:
        return measure<compact_bonsai_map<value_type, 64, lambda>>(key_fn, query_fn, capa_bits);
      default:
        break;
    }
  } else if (map_type == "plain_hash") {
    return measure<plain_hash_map<value_type, lambda>>(key_fn, query_fn, capa_bits);
  } else if (map_type == "compact_hash") {
    switch (chunk_size) {
      case 8:
        return measure<compact_hash_map<value_type, 8, lambda>>(key_fn, query_fn, capa_bits);
      case 16:
        return measure<compact_hash_map<value_type, 16, lambda>>(key_fn, query_fn, capa_bits);
      case 32:
        return measure<compact_hash_map<value_type, 32, lambda>>(key_fn, query_fn, capa_bits);
      case 64:
        return measure<compact_hash_map<value_type, 64, lambda>>(key_fn, query_fn, capa_bits);
      default:
        break;
    }
  } else if (map_type == "rrr_hash") {
    return measure<rrr_hash_map<value_type, lambda>>(key_fn, query_fn, capa_bits);
  }

  std::cerr << p.usage() << std::endl;
  return 1;
}
