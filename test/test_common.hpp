#ifndef POPLAR_TRIE_TEST_COMMON_HPP
#define POPLAR_TRIE_TEST_COMMON_HPP

#include <algorithm>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace poplar::test {

inline std::vector<std::string> make_tiny_keys() {
  return {"trie",   "denying", "defies", "defy", "tries",  "defying", "defied",
          "denied", "trying",  "deny",   "try",  "denies", "tried"};
}

inline std::vector<std::string> load_keys(const char* filename) {
  std::vector<std::string> keys;
  {
    std::ifstream ifs{filename};
    if (!ifs) {
      return {};
    }
    for (std::string line; std::getline(ifs, line);) {
      keys.push_back(line);
    }
  }
  return keys;
}

}  // namespace poplar::test

#endif  // POPLAR_TRIE_TEST_COMMON_HPP
