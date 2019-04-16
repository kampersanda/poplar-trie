#include <iostream>
#include <poplar.hpp>

int main() {
  std::vector<std::string> keys = {"Aoba", "Yun",    "Hajime", "Hihumi", "Kou",
                                   "Rin",  "Hazuki", "Umiko",  "Nene"};
  const auto num_keys = static_cast<int>(keys.size());

  poplar::plain_bonsai_map<int> map;

  try {
    for (int i = 0; i < num_keys; ++i) {
      int* ptr = map.update(keys[i]);
      *ptr = i + 1;
    }
    for (int i = 0; i < num_keys; ++i) {
      const int* ptr = map.find(keys[i]);
      if (ptr == nullptr or *ptr != i + 1) {
        return 1;
      }
      std::cout << keys[i] << ": " << *ptr << std::endl;
    }
    {
      const int* ptr = map.find("Hotaru");
      if (ptr != nullptr) {
        return 1;
      }
      std::cout << "Hotaru: " << -1 << std::endl;
    }
  } catch (const poplar::exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  std::cout << "#keys = " << map.size() << std::endl;

  return 0;
}
