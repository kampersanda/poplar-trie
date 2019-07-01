/**
 * MIT License
 *
 * Copyright (c) 2018–2019 Shunsuke Kanda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include <poplar.hpp>

int main() {
    std::vector<std::string> keys = {"Aoba", "Yun", "Hajime", "Hihumi", "Kou", "Rin", "Hazuki", "Umiko", "Nene"};
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
