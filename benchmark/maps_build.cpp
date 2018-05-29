#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

template <class t_map>
int build(const char* key_name, uint32_t capa_bits) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    return 1;
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  auto map = t_map{capa_bits};

  try {
    std::string key;
    key.reserve(1024);

    Stopwatch watch;
    while (std::getline(ifs, key)) {
      map.update(key);
      ++num_keys;
    }
    elapsed_sec = watch.sec();
  } catch (const Exception& ex) {
    std::cerr << ex.what() << std::endl;
  }

  std::cout << "name:" << short_realname<t_map>() << "\n";
  std::cout << "keys:" << num_keys << "\n";
  std::cout << "elapsed_sec:" << elapsed_sec << "\n";
  map.show_stat(std::cout);

  return 0;
}

template <size_t N = 0>
std::enable_if_t<N == NUM_MAPS, int> build_for_id(int, const char*, uint32_t) {
  return 1;
}
template <size_t N = 0>
    std::enable_if_t <
    N<NUM_MAPS, int> build_for_id(int id, const char* key_name, uint32_t capa_bits) {
  if (id - 1 == N) {
    using map_type = std::tuple_element_t<N, map_types<>>;
    return build<map_type>(key_name, capa_bits);
  }
  return build_for_id<N + 1>(id, key_name, capa_bits);
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <capa>\n";
  os << "<type>  type ID of maps\n";
  maps_list_all("  ", os);
  os << "<key>   path of input keywords\n";
  os << "<capa>  #bits of initial capacity (optional)\n";
  os.flush();
}

}  // namespace

int main(int argc, char* argv[]) {
  std::ios::sync_with_stdio(false);

  if (argc < 3 || 4 < argc) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  int map_id = std::stoi(argv[1]);
  if (map_id < 1 || NUM_MAPS < map_id) {
    show_usage(argv[0], std::cerr);
    return 1;
  }

  const char* key_name = argv[2];

  uint32_t capa_bits = 4 <= argc ? static_cast<uint32_t>(std::stoi(argv[3])) : 0;

  return build_for_id(map_id, key_name, capa_bits);
}