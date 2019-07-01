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

#include "cmdline.h"
#include "common.hpp"

namespace {

using namespace poplar;

template <class Map>
void build(const std::string& key_name, uint32_t capa_bits, uint64_t lambda, bool detail) {
    uint64_t process_size = get_process_size();

    std::ifstream ifs{key_name};
    if (!ifs) {
        std::cerr << "error: failed to open " << key_name << std::endl;
        std::exit(1);
    }

    uint64_t num_keys = 0;
    double elapsed_sec = 0.0;

    Map map{capa_bits, lambda};

    std::string key;
    key.reserve(1024);

    timer t;
    while (std::getline(ifs, key)) {
        map.update(make_char_range(key));
        ++num_keys;
    }

    elapsed_sec = t.get<>();
    process_size = get_process_size() - process_size;

#ifdef POPLAR_EXTRA_STATS
    std::cout << lambda << '\t' << process_size << '\t' << elapsed_sec << '\t' << map.rate_steps() << '\t'
              << map.num_resize() << std::endl;
#else
    std::cout << lambda << '\t' << process_size << '\t' << elapsed_sec << std::endl;
#endif

    if (detail) {
        show_member(std::cout, "", "map");
        map.show_stats(std::cout, 1);
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    cmdline::parser p;
    p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
    p.add<std::string>("map_type", 't', "cbm | cfkm", true);
    p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
    p.add<bool>("detail", 'd', "show detail stats?", false, false);
    p.parse_check(argc, argv);

    auto key_fn = p.get<std::string>("key_fn");
    auto map_type = p.get<std::string>("map_type");
    auto capa_bits = p.get<uint32_t>("capa_bits");
    auto detail = p.get<bool>("detail");

#ifdef POPLAR_EXTRA_STATS
    std::cout << "lambda\tprocess_size\telapsed_sec\trate_steps\tnum_resize" << std::endl;
#else
    std::cout << "lambda\tprocess_size\telapsed_sec" << std::endl;
#endif

    try {
        for (uint64_t lambda = 4; lambda <= 1024; lambda *= 2) {
            if (map_type == "cbm") {
                build<compact_bonsai_map<int, 16>>(key_fn, capa_bits, lambda, detail);
            }
            if (map_type == "cfkm") {
                build<compact_fkhash_map<int, 16>>(key_fn, capa_bits, lambda, detail);
            }
        }
    } catch (const exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 1;
}
