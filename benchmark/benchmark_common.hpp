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
using MapTypes =
    std::tuple<poplar::MapPP<Value, Lambda>,
               poplar::MapPE<Value, Lambda>,
               poplar::MapPG<Value, 8, Lambda>,
               poplar::MapPG<Value, 16, Lambda>,
               poplar::MapPG<Value, 32, Lambda>,
               poplar::MapPG<Value, 64, Lambda>,
               poplar::MapCP<Value, Lambda>,
               poplar::MapCE<Value, Lambda>,
               poplar::MapCG<Value, 8, Lambda>,
               poplar::MapCG<Value, 16, Lambda>,
               poplar::MapCG<Value, 32, Lambda>,
               poplar::MapCG<Value, 64, Lambda>
               >;

using HashTrieTypes =
    std::tuple<poplar::PlainHashTrie<80>,
               poplar::CompactHashTrie<80>,
               poplar::PlainHashTrie<90>,
               poplar::CompactHashTrie<90>,
               poplar::PlainHashTrie<95>,
               poplar::CompactHashTrie<95>
               >;
// clang-format on

constexpr size_t NUM_MAPS = std::tuple_size_v<MapTypes<>>;
constexpr size_t NUM_HASH_TRIES = std::tuple_size_v<HashTrieTypes>;

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
