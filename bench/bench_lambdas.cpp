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
    std::exit(1);
  }

  uint64_t num_keys = 0;
  double elapsed_sec = 0.0;

  Map map{capa_bits, lambda};

  std::string key;
  key.reserve(1024);

  timer t;
  while (std::getline(ifs, key)) {
    map.update(make_char_range(key));
    ++num_keys;
  }

  elapsed_sec = t.get<>();
  process_size = get_process_size() - process_size;

#ifdef POPLAR_EXTRA_STATS
  std::cout << lambda << '\t' << process_size << '\t' << elapsed_sec << '\t' << map.rate_steps() << std::endl;
#else
  std::cout << lambda << '\t' << process_size << '\t' << elapsed_sec << std::endl;
#endif
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  cmdline::parser p;
  p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
  p.add<std::string>("map_type", 't', "cbm | cfkm", true);
  p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
  p.parse_check(argc, argv);

  auto key_fn = p.get<std::string>("key_fn");
  auto map_type = p.get<std::string>("map_type");
  auto capa_bits = p.get<uint32_t>("capa_bits");

#ifdef POPLAR_EXTRA_STATS
  std::cout << "lambda\tprocess_size\telapsed_sec\trate_steps" << std::endl;
#else
  std::cout << "lambda\tprocess_size\telapsed_sec" << std::endl;
#endif

  try {
    for (uint64_t lambda = 4; lambda <= 1024; lambda *= 2) {
      if (map_type == "cbm") {
        build<compact_bonsai_map<int, 16>>(key_fn, capa_bits, lambda);
      }
      if (map_type == "cfkm") {
        build<compact_fkhash_map<int, 16>>(key_fn, capa_bits, lambda);
      }
    }
  } catch (const exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  return 1;
}
