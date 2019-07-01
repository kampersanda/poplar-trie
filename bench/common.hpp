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
#ifndef POPLAR_TRIE_COMMON_HPP
#define POPLAR_TRIE_COMMON_HPP

#ifdef __APPLE__
#include <mach/mach.h>
#endif
#include <cxxabi.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>

#include <poplar.hpp>

namespace poplar {

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

// From Cedar (http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/)
inline size_t get_process_size() {
#ifdef __APPLE__
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    task_info(current_task(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&t_info), &t_info_count);
    return t_info.resident_size;
#else
    FILE* fp = std::fopen("/proc/self/statm", "r");
    size_t dummy(0), vm(0);
    std::fscanf(fp, "%ld %ld ", &dummy, &vm);  // get resident (see procfs)
    std::fclose(fp);
    return vm * ::getpagesize();
#endif
}

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

}  // namespace poplar

#endif  // POPLAR_TRIE_COMMON_HPP
