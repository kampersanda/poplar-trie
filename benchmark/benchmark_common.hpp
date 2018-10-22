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
