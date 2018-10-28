# Poplar-trie: A C++ implementation of memory-efficient dynamic tries

Poplar-trie is a C++17 library of associative arrays with string keys based on a dynamic path-decomposed trie (DynPDT) described in the paper [*Practical implementation of space-efficient dynamic keyword dictionaries*](https://link.springer.com/chapter/10.1007%2F978-3-319-67428-5_19), published in SPIRE 2017 [[paper](https://sites.google.com/site/shnskknd/SPIRE2017.pdf)] [[slide](https://www.slideshare.net/ShunsukeKanda1/practical-implementation-of-spaceefficient-dynamic-keyword-dictionaries)].
However, the implementation of this library is enhanced from the conference version.

The technical details are now being written.

## Implementation overview

Poplar-trie implements a dynamically-updatable associative array mapping key strings to values of any type like `std::map<std::string,V>`.
The underlying data structure is DynPDT.
It is a customized trie structure that has additional string labels for each node.
The string labels are stored separately from the trie representation, so we need to store their pointers associated with each node.

Poplar-trie implements the associative array based on DynPDT by combining efficient trie representations with proper data structures for the string labels.
Some classes are included in this libary as follows.

### Bonsai-trie based implementations

Classes `plain_bonsai_trie` and `compact_bonsai_trie` implements trie representations based on [m-Bonsai](https://github.com/Poyias/mBonsai) techniques proposed by Poyias et al.
Selfexplanatorily, the former is a fast version and the latter is a compact version.

For the representations, two data structures for string labels are implemented as classes `plain_label_store_bt` and `compact_label_store_bt`.

### Hash-trie based implementations

Classes `plain_hash_trie` and `compact_hash_trie` implements trie representations based on [HashTrie](https://github.com/tudocomp/tudocomp) techniques proposed by Fischer and Köppl.

For the representations, three data structures for string labels are implemented as classes `plain_label_store_ht`, `compact_label_store_ht` and `rrr_label_store_ht`.


### Aliases

Class `map` implements the associative array while taking the above classes as the template arguments.
That is, there are some implementation patterns.
But, you can easily get the implementations since `poplar.hpp` provides the following aliases:

| Alias | Trie | String Label |
|:--|:--|:--|
|`plain_bonsai_map`|`plain_bonsai_trie`|`plain_label_store_bt`|
|`compact_bonsai_map`|`compact_bonsai_trie`|`compact_label_store_bt`|
|`plain_hash_map`|`plain_hash_trie`|`plain_label_store_ht`|
|`compact_hash_map`|`compact_hash_trie`|`compact_label_store_ht`|
|`rrr_hash_map`|`compact_hash_trie`|`rrr_label_store_ht`|

These have template argument `Lambda` in common.
This is a parameter depending on lengths of given strings.
From previous experimental results, the value 16 (default) would be good for natural language words.
For long strings such as URLs, the value 32 or 64 would be good.

## Install

Please through the path to the directory `poplar-trie/include`.
This library consists of only header files.

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

  poplar::plain_hash_map<int> map;

  try {
    for (int i = 0; i < num_keys; ++i) {
      int* ptr = map.update(poplar::make_char_range(keys[i]));
      *ptr = i + 1;
    }
    for (int i = 0; i < num_keys; ++i) {
      const int* ptr = map.find(poplar::make_char_range(keys[i]));
      if (ptr == nullptr or *ptr != i + 1) {
        return 1;
      }
      std::cout << keys[i] << ": " << *ptr << std::endl;
    }
    {
      const int* ptr = map.find(poplar::make_char_range("Hotaru"));
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

## Benchmarks

The main advantage of Poplar-trie is high space efficiency as can be seen in the following results.

The experiments were carried out on Intel Core i7 @3.5 GHz CPU, with 16 GB of RAM, running Mac OS X 10.13.
The codes were compiled using Apple LLVM version 9.1.0 with optimization -O3.
The dictionaries were constructed by inserting all page titles from Japanese Wikipedia (38.3 MiB) in random order.
The value type is `int`.
The maximum resident set size during construction was measured using the `/usr/bin/time` command.
The insertion time was also measured using `std::chrono::duration_cast`.
And, search time for the same strings was measured.

| Implementation | Space<br>(MiB) | Insert<br>(micros/key) | Search<br>(micros/key) |
|--------------------------|------------:|-------------------:|---------------------:|
| std::map | 139.6 | 1.45 | 1.65 |
| std::unordered\_map | 162.3 | 0.66 | 0.28 |
| [google::dense\_hash\_map](https://github.com/sparsehash/sparsehash) | 291.4 | 0.42 | 0.09 |
| [google::sparse\_hash\_map](https://github.com/sparsehash/sparsehash) | 185.3 | 4.41 | 0.19 |
| [JudySL](http://judy.sourceforge.net) | 83.4 | 0.72 | 0.60 |
| [hat-trie](https://github.com/dcjones/hat-trie) | 77.1 | 0.84 | 0.21 |
| [libart](https://github.com/armon/libart) | 149.4 | 0.77 | 0.79 |
| [cedar (reduced)](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | 266.7 | 1.03 | 0.55 |
| [cedar (prefix)](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | 154.2 | 0.86 | 0.59 |
| poplar::plain\_bonsai\_map | 86.3 | 0.72 | 0.61 |
| poplar::compact\_bonsai\_map | **44.4** | 1.67 | 0.85 |
| poplar::plain\_hash\_map | 63.3 | 0.65 | 0.73 |
| poplar::compact\_hash\_map | **45.3** | 0.93 | 0.96 |
| poplar::rrr\_hash\_map | **44.3** | 1.29 | 1.48 |

## Todo

- Support the deletion operation
- Improve the time performance
- Add comments to the codes
- Create the API document

## Related work

- [compact\_sparse\_hash](https://github.com/tudocomp/compact_sparse_hash) is an efficient implementation of a compact associative array with integer keys.
- [mBonsai](https://github.com/Poyias/mBonsai) is the original implementation of succinct dynamic tries.
- [tudocomp](https://github.com/tudocomp/tudocomp) includes many dynamic trie implementations for LZ factorization.
 
## Special thanks

Thanks to [Dr. Dominik Köppl](https://github.com/koeppl) I was able to create the bijective hash function in `bijective_hash.hpp`.

