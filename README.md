# Poplar-trie: A C++17 implementation of memory-efficient dynamic tries

Poplar-trie is a C++17 library of a memory-efficient associative array whose keys are strings. The data structure is based on a dynamic path-decomposed trie (DynPDT) described in the paper, Shunsuke Kanda, Dominik Köppl, Yasuo Tabei, Kazuhiro Morita, and Masao Fuketa: [Dynamic Path-decomposed Tries](https://arxiv.org/abs/1906.06015), *ACM Journal of Experimental Algorithmics (JEA)*, *25*(1): 1–28, 2020.

## Implementation overview

Poplar-trie is a memory-efficient updatable associative array implementation which maps key strings to values of any type like `std::map<std::string,anytype>`.
DynPDT is composed of two structures: dynamic trie and node label map (NLM) structures.
This library contains some implementations of those structures, as follows.

### Implementations based on m-Bonsai

- Classes [`plain_bonsai_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_bonsai_trie.hpp) and [`compact_bonsai_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_bonsai_trie.hpp) are dynamic trie implementations based on [m-Bonsai](https://github.com/Poyias/mBonsai).
- Classes [`plain_bonsai_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_bonsai_nlm.hpp) and [`compact_bonsai_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_bonsai_nlm.hpp) are NLM implementations designed for these dynamic tries.

### Implementations based on FK-hash

- Classes [`plain_fkhash_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_fkhash_trie.hpp) and [`compact_fkhash_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_fkhash_trie.hpp) are dynamic trie implementations based on [HashTrie](https://github.com/tudocomp/tudocomp) developed by Fischer and Köppl.
- Classes [`plain_fkhash_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_fkhash_nlm.hpp) and [`compact_fkhash_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_fkhash_nlm.hpp) are NLM implementations designed for these dynamic tries.

### Aliases

Class [`map`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/map.hpp) takes these classes as the template arguments and implements the associative array.
So, there are some implementation combinations.
In [`poplar.hpp`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar.hpp), the following aliases are provided.

| Alias                     | Trie Impl.            | NLM impl.            |
| :------------------------ | :-------------------- | :------------------- |
| `plain_bonsai_map`        | `plain_bonsai_trie`   | `plain_bonsai_nlm`   |
| `semi_compact_bonsai_map` | `plain_bonsai_trie`   | `compact_bonsai_nlm` |
| `compact_bonsai_map`      | `compact_bonsai_trie` | `compact_bonsai_nlm` |
| `plain_fkhash_map`        | `plain_fkhash_trie`   | `plain_fkhash_nlm`   |
| `semi_compact_fkhash_map` | `plain_fkhash_trie`   | `compact_fkhash_nlm` |
| `compact_fkhash_map`      | `compact_fkhash_trie` | `compact_fkhash_nlm` |


## Install

This library consists of only header files.
Please through the path to the directory [`poplar-trie/include`](https://github.com/kampersanda/poplar-trie/tree/master/include).


## Build instructions

You can download and compile Poplar-trie as the following commands.

```
$ git clone https://github.com/kampersanda/poplar-trie.git
$ cd poplar-trie
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

The library uses C++17, so please install g++ 7.0 (or greater) or clang 4.0 (or greater).
In addition, CMake 2.8 (or greater) has to be installed to compile the library.

On the default setting, the library attempts to use `SSE4.2` for popcount primitives.
If you do not want to use it, please set `DISABLE_SSE4_2` at build time, e.g., `cmake .. -DDISABLE_SSE4_2=1`.

## Easy example

The following code is an easy example of inserting and searching key-value pairs.

```c++
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
```

The output will be

```
Aoba: 1
Yun: 2
Hajime: 3
Hihumi: 4
Kou: 5
Rin: 6
Hazuki: 7
Umiko: 8
Nene: 9
Hotaru: -1
#keys = 9
```

### Note: Deletion implementation

Since DynPDT cannot support garbage collection for deleted keys, Poplar-trie does not provide deletion functions. However, you can easily implement that function by setting the value associated with a deleted key to an invalid value. For example,

```c++
int* ptr = map.update(deleted_key);
*ptr = -1; // invalid value
```

In this approach, the memory used for deleted keys is not released, although it may be reused for keys inserted subsequently.

## Benchmarks

Comparison experiments were conducted on one core of a quad-core Intel Xeon CPU E5-2680 v2 clocked at 2.80 Ghz in a machine with 256 GB of RAM, running the 64-bit version of CentOS 6.10 based on Linux 2.6.
The source code was compiled with g++ (version 7.3.0) in optimization mode -O3.

To measure the performance, we inserted strings in a dataset to a data structure in random order, and measured the maximum resident set size and insertion time.
The lookup time was measured by retrieving a million strings randomly extracted from the dataset.

The source codes for the experiments are at [dictionary_bench](https://github.com/kampersanda/dictionary_bench).

### Page Titles of English Wikipedia

- Dataset: All page titles from English Wikipedia in Sep. 2018
- Number of keys: 14,130,439
- File size: 0.28 GiB

| Implementation                                                                   | Space (GiB) | Insert (us/key) | Lookup (us/key) |
| -------------------------------------------------------------------------------- | ----------: | --------------: | --------------: |
| [`poplar::plain_bonsai_map`](https://github.com/kampersanda/poplar-trie)         |        0.64 |            0.98 |            0.68 |
| [`poplar::semi_compact_bonsai_map`](https://github.com/kampersanda/poplar-trie)  |        0.28 |            1.60 |            0.96 |
| [`poplar::compact_bonsai_map`](https://github.com/kampersanda/poplar-trie)       |        0.24 |            1.71 |            1.02 |
| [`poplar::plain_fkhash_map`](https://github.com/kampersanda/poplar-trie)         |        0.67 |            0.79 |            0.86 |
| [`poplar::semi_compact_fkhash_map`](https://github.com/kampersanda/poplar-trie)  |        0.31 |            0.96 |            1.15 |
| [`poplar::compact_fkhash_map`](https://github.com/kampersanda/poplar-trie)       |        0.27 |            1.14 |            1.22 |
| [`std::unordered_map`](http://en.cppreference.com/w/cpp/container/unordered_map) |        1.29 |            0.50 |            0.27 |
| [`google::dense_hash_map`](https://github.com/sparsehash/sparsehash)             |        1.64 |            0.54 |            0.14 |
| [`spp::sparse_hash_map`](https://github.com/greg7mdp/sparsepp)                   |        0.97 |            0.69 |            0.18 |
| [`tsl::hopscotch_map`](https://github.com/Tessil/hopscotch-map)                  |        1.08 |            0.42 |            0.13 |
| [`tsl::robin_map`](https://github.com/Tessil/robin-map)                          |        1.83 |            0.41 |            0.12 |
| [`tsl::array_map`](https://github.com/Tessil/array-hash)                         |        0.69 |            0.73 |            0.14 |
| [`tsl::htrie_map`](https://github.com/Tessil/hat-trie)                           |        0.43 |            0.60 |            0.27 |
| [`JudySL`](http://judy.sourceforge.net)                                          |        0.66 |            0.92 |            0.74 |
| [`libart`](https://github.com/armon/libart)                                      |        1.23 |            1.00 |            0.73 |
| [`cedar::da`](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) (reduced trie)     |        1.19 |            0.89 |            0.59 |
| [`cedar::da`](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) (prefix trie)      |        0.63 |            0.89 |            0.61 |

### URLs of UK domain

- Dataset: URLs obtained from a 2005 crawl of the `.uk` domain performed by UbiCrawler
- Number of keys: 39,459,925
- File size: 2.7 GiB

| Implementation                                                                   | Space (GiB) | Insert (us/key) | Lookup (us/key) |
| -------------------------------------------------------------------------------- | ----------: | --------------: | --------------: |
| [`poplar::plain_bonsai_map`](https://github.com/kampersanda/poplar-trie)         |        2.32 |            1.45 |            0.94 |
| [`poplar::semi_compact_bonsai_map`](https://github.com/kampersanda/poplar-trie)  |        1.26 |            2.76 |            1.44 |
| [`poplar::compact_bonsai_map`](https://github.com/kampersanda/poplar-trie)       |        1.09 |            2.87 |            1.44 |
| [`poplar::plain_fkhash_map`](https://github.com/kampersanda/poplar-trie)         |        2.32 |            1.27 |            1.24 |
| [`poplar::semi_compact_fkhash_map`](https://github.com/kampersanda/poplar-trie)  |        1.38 |            1.74 |            1.93 |
| [`poplar::compact_fkhash_map`](https://github.com/kampersanda/poplar-trie)       |        1.21 |            2.04 |            2.02 |
| [`std::unordered_map`](http://en.cppreference.com/w/cpp/container/unordered_map) |        6.05 |            0.67 |            0.50 |
| [`google::dense_hash_map`](https://github.com/sparsehash/sparsehash)             |       10.50 |            1.09 |            0.27 |
| [`spp::sparse_hash_map`](https://github.com/greg7mdp/sparsepp)                   |        5.06 |            0.96 |            0.37 |
| [`tsl::hopscotch_map`](https://github.com/Tessil/hopscotch-map)                  |        6.23 |            0.75 |            0.25 |
| [`tsl::robin_map`](https://github.com/Tessil/robin-map)                          |        9.23 |            0.63 |            0.25 |
| [`tsl::array_map`](https://github.com/Tessil/array-hash)                         |        5.91 |            1.16 |            0.28 |
| [`tsl::htrie_map`](https://github.com/Tessil/hat-trie)                           |        2.68 |            1.08 |            0.51 |
| [`JudySL`](http://judy.sourceforge.net)                                          |        2.21 |            1.88 |            1.59 |
| [`libart`](https://github.com/armon/libart)                                      |        5.17 |            1.64 |            1.19 |
| [`cedar::da`](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) (reduced trie)     |        7.37 |            2.24 |            2.30 |
| [`cedar::da`](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) (prefix trie)      |        2.02 |            2.20 |            2.28 |

## Todo

- Add comments to the codes
- Create the API document

## Licensing

This library is free software provided under [MIT License](https://github.com/kampersanda/poplar-trie/blob/master/LICENSE).

If you use the library, please cite the following paper:

```tex
@article{kanda2020dynamic,
  title={Dynamic Path-decomposed Tries},
  author={Kanda, Shunsuke and K{\"o}ppl, Dominik and Tabei, Yasuo and Morita, Kazuhiro and Fuketa, Masao},
  journal={Journal of Experimental Algorithmics (JEA)},
  volume={25},
  number={1},
  pages={1--28},
  year={2020},
  publisher={ACM}
}
```

## Related work

- [compact\_sparse\_hash](https://github.com/tudocomp/compact_sparse_hash) is an efficient implementation of a compact associative array with integer keys.
- [mBonsai](https://github.com/Poyias/mBonsai) is the original implementation of succinct dynamic tries.
- [tudocomp](https://github.com/tudocomp/tudocomp) includes many dynamic trie implementations for LZ factorization.

## Special thanks

Thanks to [Dr. Dominik Köppl](https://github.com/koeppl) I was able to create the bijective hash function in `bijective_hash.hpp`.
