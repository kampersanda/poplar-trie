#ifndef POPLAR_TRIE_BENCHMARK_COMMON_HPP
#define POPLAR_TRIE_BENCHMARK_COMMON_HPP

#include <cxxabi.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <poplar.hpp>

namespace poplar::benchmark {

class Stopwatch {
public:
  using hrc = std::chrono::high_resolution_clock;

  Stopwatch() = default;

  double sec() const {
    return std::chrono::duration<double>(hrc::now() - tp_).count();
  }
  double milli_sec() const {
    return std::chrono::duration<double, std::milli>(hrc::now() - tp_).count();
  }
  double micro_sec() const {
    return std::chrono::duration<double, std::micro>(hrc::now() - tp_).count();
  }

private:
  hrc::time_point tp_{hrc::now()};
};

template <typename T>
inline std::string realname() {
  int status;
  const auto& id = typeid(T);
  return abi::__cxa_demangle(id.name(), 0, 0, &status);
}

using value_type = int;
static constexpr uint64_t LAMBDA = 16;

using map_types = std::tuple<
  poplar::MapPP<value_type, LAMBDA>,
  poplar::MapPE<value_type, LAMBDA>,
  poplar::MapPG<value_type, 8, LAMBDA>,
  poplar::MapPG<value_type, 16, LAMBDA>,
  poplar::MapPG<value_type, 32, LAMBDA>,
  poplar::MapPG<value_type, 64, LAMBDA>,
  poplar::MapCP<value_type, LAMBDA>,
  poplar::MapCE<value_type, LAMBDA>,
  poplar::MapCG<value_type, 8, LAMBDA>,
  poplar::MapCG<value_type, 16, LAMBDA>,
  poplar::MapCG<value_type, 32, LAMBDA>,
  poplar::MapCG<value_type, 64, LAMBDA>
>;

constexpr size_t NUM_MAPS = std::tuple_size<map_types>::value;

template<size_t N = NUM_MAPS>
inline void maps_list_all(const char* pfx, std::ostream& os) {
  maps_list_all<N - 1>(pfx, os);
  using map_type = std::tuple_element_t<N - 1, map_types>;
  os << pfx << std::setw(2) << N << ": " << realname<map_type>() << "\n";
}
template<>
inline void maps_list_all<1>(const char* pfx, std::ostream& os) {
  using map_type = std::tuple_element_t<0, map_types>;
  os << pfx << std::setw(2) << 1 << ": " << realname<map_type>() << "\n";
}

template <size_t N>
inline double get_average(const std::array<double, N>& ary) {
  double sum = 0.0;
  for (auto v : ary) {
    sum += v;
  }
  return sum / N;
}

inline std::vector<std::string> load_keys(const char* key_name) {
  std::ifstream ifs{key_name};
  if (!ifs) {
    std::cerr << "Error: failed to open " << key_name << std::endl;
    exit(1);
  }

  std::vector<std::string> keys;
  for (std::string line; std::getline(ifs, line);) {
    keys.push_back(line);
  }

  return keys;
}

}

#endif //POPLAR_TRIE_BENCHMARK_COMMON_HPP
