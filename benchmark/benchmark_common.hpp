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
template <typename T>
inline std::string short_realname() {
  auto name = realname<T>();
  name = std::regex_replace(name, std::regex{R"( |poplar::)"}, "");
  name = std::regex_replace(name, std::regex{R"((\d+)ul{0,2})"}, "$1");
  return name;
}

// clang-format off
template <typename Value = int, uint64_t Lambda = 16>
using map_types = std::tuple<poplar::plain_hash_map<Value, Lambda>,
                             poplar::compact_hash_map<Value, 8, Lambda>,
                             poplar::compact_hash_map<Value, 16, Lambda>,
                             poplar::compact_hash_map<Value, 32, Lambda>,
                             poplar::compact_hash_map<Value, 64, Lambda>,
                             poplar::plain_bonsai_map<Value, Lambda>,
                             poplar::compact_bonsai_map<Value, 8, Lambda>,
                             poplar::compact_bonsai_map<Value, 16, Lambda>,
                             poplar::compact_bonsai_map<Value, 32, Lambda>,
                             poplar::compact_bonsai_map<Value, 64, Lambda>>;
// clang-format on

constexpr size_t NUM_MAPS = std::tuple_size_v<map_types<>>;

template <typename Types, size_t N = std::tuple_size_v<Types>>
constexpr void list_all(const char* pfx, std::ostream& os) {
  static_assert(N != 0);

  if constexpr (N > 1) {
    list_all<Types, N - 1>(pfx, os);
  }
  using type = std::tuple_element_t<N - 1, Types>;
  os << pfx << std::setw(2) << N << ": " << short_realname<type>() << "\n";
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
