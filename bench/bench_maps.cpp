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

using value_type = int;

inline double get_average(const std::vector<double>& ary) {
    double sum = 0.0;
    for (auto v : ary) {
        sum += v;
    }
    return sum / ary.size();
}
inline double get_min(const std::vector<double>& ary) {
    double min = std::numeric_limits<double>::max();
    for (auto v : ary) {
        if (v < min) {
            min = v;
        }
    }
    return min;
}

template <class Map>
int bench(const cmdline::parser& p) {
    auto key_fn = p.get<std::string>("key_fn");
    auto query_fn = p.get<std::string>("query_fn");
    auto capa_bits = p.get<uint32_t>("capa_bits");
    auto lambda = p.get<uint64_t>("lambda");
    auto runs = p.get<int>("runs");
    auto detail = p.get<bool>("detail");

    uint64_t num_keys = 0, num_queries = 0;
    uint64_t ok = 0, ng = 0;
    uint64_t process_size = get_process_size();

    double insert_us_per_key = 0.0, search_us_per_query = 0.0;
    double best_insert_us_per_key = 0.0, best_search_us_per_query = 0.0;

    auto map = std::make_unique<Map>(capa_bits, lambda);
    {
        std::ifstream ifs{key_fn};
        if (!ifs) {
            std::cerr << "error: failed to open " << key_fn << std::endl;
            return 1;
        }

        std::string key;
        key.reserve(1024);

        while (std::getline(ifs, key)) {
            *map->update(key) = 1;
            ++num_keys;
        }
        process_size = get_process_size() - process_size;
    }

    std::shared_ptr<std::vector<std::string>> keys;
    std::shared_ptr<std::vector<std::string>> queries;

    {
        std::ifstream ifs{key_fn};
        if (!ifs) {
            std::cerr << "Error: failed to open " << key_fn << std::endl;
            return 1;
        }

        keys = std::make_shared<std::vector<std::string>>();
        keys->reserve(num_keys);

        for (std::string line; std::getline(ifs, line);) {
            keys->push_back(line);
        }
    }

    if (query_fn != "-") {
        std::ifstream ifs{query_fn};
        if (!ifs) {
            std::cerr << "Error: failed to open " << query_fn << std::endl;
            return 1;
        }

        queries = std::make_shared<std::vector<std::string>>();

        for (std::string line; std::getline(ifs, line);) {
            queries->push_back(line);
        }
        queries->shrink_to_fit();
    } else {
        queries = keys;
    }

    {
        std::vector<double> insert_times(runs);
        std::vector<double> search_times(runs);

        for (int i = 0; i < runs; ++i) {
            auto map = std::make_unique<Map>(capa_bits, lambda);

            // insertion
            {
                timer t;
                for (const std::string& key : *keys) {
                    *map->update(key) = 1;
                }
                insert_times[i] = t.get<std::micro>() / keys->size();
            }

            // retrieval
            size_t _ok = 0, _ng = 0;
            {
                timer t;
                for (const std::string& query : *queries) {
                    auto ptr = map->find(query);
                    if (ptr != nullptr and *ptr == 1) {
                        ++_ok;
                    } else {
                        ++_ng;
                    }
                }
                search_times[i] = t.get<std::micro>() / queries->size();
            }

            if (i != 0) {
                if ((ok != _ok) or (ng != _ng)) {
                    std::cerr << "critical error for search results" << std::endl;
                    return 1;
                }
            }

            ok = _ok;
            ng = _ng;
        }

        num_keys = keys->size();
        num_queries = queries->size();
        insert_us_per_key = get_average(insert_times);
        best_insert_us_per_key = get_min(insert_times);
        search_us_per_query = get_average(search_times);
        best_search_us_per_query = get_min(search_times);
    }

    std::ostream& out = std::cout;
    auto indent = get_indent(0);

    show_stat(out, indent, "map_name", short_realname<Map>());
    show_stat(out, indent, "key_fn", key_fn);
    show_stat(out, indent, "query_fn", query_fn);
    show_stat(out, indent, "init_capa_bits", capa_bits);

    show_stat(out, indent, "rss_bytes", process_size);
    show_stat(out, indent, "rss_MiB", process_size / (1024.0 * 1024.0));

    show_stat(out, indent, "num_keys", num_keys);
    show_stat(out, indent, "num_queries", num_queries);

    show_stat(out, indent, "runs", runs);
    show_stat(out, indent, "insert_us_per_key", insert_us_per_key);
    show_stat(out, indent, "best_insert_us_per_key", best_insert_us_per_key);
    show_stat(out, indent, "search_us_per_query", search_us_per_query);
    show_stat(out, indent, "best_search_us_per_query", best_search_us_per_query);

    show_stat(out, indent, "ok", ok);
    show_stat(out, indent, "ng", ng);

    if (detail) {
        show_member(out, indent, "map");
        map->show_stats(out, 1);
    }

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    cmdline::parser p;
    p.add<std::string>("key_fn", 'k', "input file name of keywords", true);
    p.add<std::string>("query_fn", 'q', "input file name of queries", false, "-");
    p.add<std::string>("map_type", 't', "pbm | scbm | cbm | pfkm | scfkm | cfkm", true);
    p.add<uint32_t>("chunk_size", 'c', "8 | 16 | 32 | 64 (for scbm, cbm, scfkm and cfkm)", false, 16);
    p.add<uint32_t>("capa_bits", 'b', "#bits of initial capacity", false, 16);
    p.add<uint64_t>("lambda", 'l', "lambda", false, 32);
    p.add<int>("runs", 'r', "# of runs", false, 10);
    p.add<bool>("detail", 'd', "show detail stats?", false, false);
    p.parse_check(argc, argv);

    auto map_type = p.get<std::string>("map_type");
    auto chunk_size = p.get<uint32_t>("chunk_size");

    try {
        if (map_type == "pbm") {
            return bench<plain_bonsai_map<value_type>>(p);
        }
        if (map_type == "scbm") {
            switch (chunk_size) {
                case 8:
                    return bench<semi_compact_bonsai_map<value_type, 8>>(p);
                case 16:
                    return bench<semi_compact_bonsai_map<value_type, 16>>(p);
                case 32:
                    return bench<semi_compact_bonsai_map<value_type, 32>>(p);
                case 64:
                    return bench<semi_compact_bonsai_map<value_type, 64>>(p);
                default:
                    std::cerr << p.usage() << std::endl;
                    return 1;
            }
        }
        if (map_type == "cbm") {
            switch (chunk_size) {
                case 8:
                    return bench<compact_bonsai_map<value_type, 8>>(p);
                case 16:
                    return bench<compact_bonsai_map<value_type, 16>>(p);
                case 32:
                    return bench<compact_bonsai_map<value_type, 32>>(p);
                case 64:
                    return bench<compact_bonsai_map<value_type, 64>>(p);
                default:
                    std::cerr << p.usage() << std::endl;
                    return 1;
            }
        }
        if (map_type == "pfkm") {
            return bench<plain_fkhash_map<value_type>>(p);
        }
        if (map_type == "scfkm") {
            switch (chunk_size) {
                case 8:
                    return bench<semi_compact_fkhash_map<value_type, 8>>(p);
                case 16:
                    return bench<semi_compact_fkhash_map<value_type, 16>>(p);
                case 32:
                    return bench<semi_compact_fkhash_map<value_type, 32>>(p);
                case 64:
                    return bench<semi_compact_fkhash_map<value_type, 64>>(p);
                default:
                    std::cerr << p.usage() << std::endl;
                    return 1;
            }
        }
        if (map_type == "cfkm") {
            switch (chunk_size) {
                case 8:
                    return bench<compact_fkhash_map<value_type, 8>>(p);
                case 16:
                    return bench<compact_fkhash_map<value_type, 16>>(p);
                case 32:
                    return bench<compact_fkhash_map<value_type, 32>>(p);
                case 64:
                    return bench<compact_fkhash_map<value_type, 64>>(p);
                default:
                    std::cerr << p.usage() << std::endl;
                    return 1;
            }
        }
    } catch (const exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    std::cerr << p.usage() << std::endl;
    return 1;
}
