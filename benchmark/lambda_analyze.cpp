#include <iostream>

#include "benchmark_common.hpp"
#include "cmdline.h"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

template <class Map>
void build(const std::string& key_name, uint32_t capa_bits) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    std::exit(1);
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  Map map{capa_bits};

  try {
    std::string key;
    key.reserve(1024);

    timer t;
    while (std::getline(ifs, key)) {
      map.update(make_char_range(key));
      ++num_keys;
    }
    elapsed_sec = t.get<>();
  } catch (const exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

#ifdef POPLAR_EXTRA_STATS
  std::cout << Map::lambda << '\t' << elapsed_sec << '\t' << map.rate_steps() << '\n';
#else
  std::cout << Map::lambda << '\t' << elapsed_sec << '\n';
#endif
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto capa_bits = p.get<uint32_t>("capa_bits");

#ifdef POPLAR_EXTRA_STATS
  std::cout << "lambda\telapsed_sec\trate_steps\n";
#else
  std::cout << "lambda\telapsed_sec\n";
#endif

  build<plain_hash_map<int, 2>>(key_fn, capa_bits);
  build<plain_hash_map<int, 4>>(key_fn, capa_bits);
  build<plain_hash_map<int, 8>>(key_fn, capa_bits);
  build<plain_hash_map<int, 16>>(key_fn, capa_bits);
  build<plain_hash_map<int, 32>>(key_fn, capa_bits);
  build<plain_hash_map<int, 64>>(key_fn, capa_bits);
  build<plain_hash_map<int, 128>>(key_fn, capa_bits);

  return 1;
}
