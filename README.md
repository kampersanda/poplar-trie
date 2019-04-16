# Poplar-trie: A C++17 implementation of memory-efficient dynamic tries

Poplar-trie is a C++17 library of an associative array whose keys are strings.
It is based on a dynamic path-decomposed trie (DynPDT) described in the paper [*Practical implementation of space-efficient dynamic keyword dictionaries*](https://link.springer.com/chapter/10.1007%2F978-3-319-67428-5_19), published in SPIRE 2017 [[paper](https://sites.google.com/site/shnskknd/SPIRE2017.pdf)] [[slide](https://www.slideshare.net/ShunsukeKanda1/practical-implementation-of-spaceefficient-dynamic-keyword-dictionaries)].
However, the implementation of this library is enhanced from the conference version.

The technical details are now being written.

## Implementation overview

Poplar-trie is a space-efficient updatable associative array implementation which maps key strings to values of any type like `std::map<std::string,anytype>`.
DynPDT is composed of two structures: dynamic trie and node label map (NLM) structures.
This library contains some implementations for the structures, as follows.

### Implementations based on m-Bonsai

- Classes [`plain_bonsai_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_bonsai_trie.hpp) and [`compact_bonsai_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_bonsai_trie.hpp) are dynamic trie implementations based on [m-Bonsai](https://github.com/Poyias/mBonsai).
- Classes [`plain_bonsai_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_bonsai_nlm.hpp) and [`compact_bonsai_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_bonsai_nlm.hpp) are NLM implementations designed for these implementations.

### Implementations based on FK-hash

- Classes [`plain_fkhash_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_fkhash_trie.hpp) and [`compact_fkhash_trie`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_fkhash_trie.hpp) are dynamic trie implementations based on [HashTrie](https://github.com/tudocomp/tudocomp) developed by Fischer and Köppl.
- Classes [`plain_fkhash_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/plain_fkhash_nlm.hpp) and [`compact_fkhash_nlm`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/compact_fkhash_nlm.hpp) are NLM implementations designed for these implementations.

### Aliases

Class [`map`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar/map.hpp) takes these classes as the template arguments and implements the associative array.
So, there are some implementation combinations.
In [`poplar.hpp`](https://github.com/kampersanda/poplar-trie/blob/master/include/poplar.hpp), the following aliases are provided.

| Alias | Trie Impl. | NLM impl. |
|:--|:--|:--|
|`plain_bonsai_map`|`plain_bonsai_trie`|`plain_bonsai_nlm`|
|`semi_compact_bonsai_map`|`plain_bonsai_trie`|`compact_bonsai_nlm`|
|`compact_bonsai_map`|`compact_bonsai_trie`|`compact_bonsai_nlm`|
|`plain_fkhash_map`|`plain_fkhash_trie`|`plain_fkhash_nlm`|
|`semi_compact_fkhash_map`|`plain_fkhash_trie`|`compact_fkhash_nlm`|
|`compact_fkhash_map`|`compact_fkhash_trie`|`compact_fkhash_nlm`|


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

<!--## Benchmarks

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
-->

## Todo

- Support the deletion operation
- Add comments to the codes
- Create the API document

## Related work

- [compact\_sparse\_hash](https://github.com/tudocomp/compact_sparse_hash) is an efficient implementation of a compact associative array with integer keys.
- [mBonsai](https://github.com/Poyias/mBonsai) is the original implementation of succinct dynamic tries.
- [tudocomp](https://github.com/tudocomp/tudocomp) includes many dynamic trie implementations for LZ factorization.
 
## Special thanks

Thanks to [Dr. Dominik Köppl](https://github.com/koeppl) I was able to create the bijective hash function in `bijective_hash.hpp`.

