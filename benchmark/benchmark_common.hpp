#ifndef POPLAR_TRIE_BENCHMARK_COMMON_HPP
#define POPLAR_TRIE_BENCHMARK_COMMON_HPP

#include <cxxabi.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>

#include <poplar.hpp>

namespace poplar::benchmark {

class timer {
 public:
  using hrc = std::chrono::high_resolution_clock;

  timer() = default;

  template <class Period = std::ratio<1>>
  double get() const {
    return std::chrono::duration<double, Period>(hrc::now() - tp_).count();
  }

 private:
  hrc::time_point tp_{hrc::now()};
};

template <typename T>
inline std::string realname() {
  int status;
  return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
}

// clang-format off
template <typename Value = int, uint64_t Lambda = 16>
using map_types = std::tuple<poplar::map_pp<Value, Lambda>,
                             poplar::map_pc<Value, 8, Lambda>,
                             poplar::map_pc<Value, 16, Lambda>,
                             poplar::map_pc<Value, 32, Lambda>,
                             poplar::map_pc<Value, 64, Lambda>,
                             poplar::map_cp<Value, Lambda>,
                             poplar::map_cc<Value, 8, Lambda>,
                             poplar::map_cc<Value, 16, Lambda>,
                             poplar::map_cc<Value, 32, Lambda>,
                             poplar::map_cc<Value, 64, Lambda>,
                             poplar::map_pp_r<Value, Lambda>,
                             poplar::map_pc_r<Value, 8, Lambda>,
                             poplar::map_pc_r<Value, 16, Lambda>,
                             poplar::map_pc_r<Value, 32, Lambda>,
                             poplar::map_pc_r<Value, 64, Lambda>,
                             poplar::map_cp_r<Value, Lambda>,
                             poplar::map_cc_r<Value, 8, Lambda>,
                             poplar::map_cc_r<Value, 16, Lambda>,
                             poplar::map_cc_r<Value, 32, Lambda>,
                             poplar::map_cc_r<Value, 64, Lambda>>;
// clang-format on

constexpr size_t NUM_MAPS = std::tuple_size_v<map_types<>>;

template <typename Types, size_t N = std::tuple_size_v<Types>>
constexpr void list_all(const char* pfx, std::ostream& os) {
  static_assert(N != 0);

  if constexpr (N > 1) {
    list_all<Types, N - 1>(pfx, os);
  }
  using type = std::tuple_element_t<N - 1, Types>;
  os << pfx << std::setw(2) << N << ": " << realname<type>() << "\n";
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

}  // namespace poplar::benchmark

#endif  // POPLAR_TRIE_BENCHMARK_COMMON_HPP
