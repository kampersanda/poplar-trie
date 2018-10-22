#include <iostream>

#include "benchmark_common.hpp"

namespace {

using namespace poplar;
using namespace poplar::benchmark;

template <class Map>
int build(const char* key_name, uint32_t capa_bits) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "error: failed to open " << key_name << std::endl;
    return 1;
  }

  size_t num_keys = 0;
  double elapsed_sec = 0.0;

  auto map = Map{capa_bits};

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

  std::ostream& out = std::cout;
  auto indent = get_indent(0);

  show_stat(out, indent, "map_name", realname<Map>());
  show_stat(out, indent, "key_name", key_name);
  show_stat(out, indent, "init_capa_bits", capa_bits);
  show_stat(out, indent, "num_keys", num_keys);
  show_stat(out, indent, "elapsed_sec", elapsed_sec);

  show_stat(out, indent, "map");
  map.show_stats(out, 1);

  return 0;
}

template <size_t N = 0>
int build_with_id(int id, const char* key_name, uint32_t capa_bits) {
  if constexpr (N >= NUM_MAPS) {
    return 1;
  } else {
    if (id - 1 == N) {
      return build<std::tuple_element_t<N, map_types<>>>(key_name, capa_bits);
    }
    return build_with_id<N + 1>(id, key_name, capa_bits);
  }
}

void show_usage(const char* exe, std::ostream& os) {
  os << exe << " <type> <key> <capa>\n";
  os << "<type>  type ID of maps\n";
  list_all<map_types<>>("  ", os);
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
  if (map_id < 1 or NUM_MAPS < map_id) {
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