#include <iostream>
#include <poplar.hpp>

int main() {
  std::vector<std::string> keys = {
    "Aoba", "Yun", "Hajime", "Hihumi",
    "Kou", "Rin", "Hazuki", "Umiko", "Nene"
  };

  poplar::MapPP<int> map;

  try {
    for (int i = 0; i < keys.size(); ++i) {
      int* ptr = map.update(keys[i]);
      *ptr = i + 1;
    }
    for (int i = 0; i < keys.size(); ++i) {
      const int* ptr = map.find(keys[i]);
      std::cout << keys[i] << ": " << *ptr << std::endl;
    }
    {
      const int* ptr = map.find("Hotaru");
      if (ptr == nullptr) {
        std::cout << "Hotaru: " << -1 << std::endl;
      } else {
        return 1;
      }
    }
  } catch (const poplar::Exception& ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  std::cout << "# of keys is " << map.size() << std::endl;

  return 0;
}
