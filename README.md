# Poplar-trie

Poplar-trie is a C++17 library of associative arrays with string keys based on a dynamic path-decomposed trie described in the paper [*Practical implementation of space-efficient dynamic keyword dictionaries*](https://link.springer.com/chapter/10.1007%2F978-3-319-67428-5_19), published in SPIRE 2017 [[paper](https://sites.google.com/site/shnskknd/SPIRE2017.pdf)] [[slide](https://www.slideshare.net/ShunsukeKanda1/practical-implementation-of-spaceefficient-dynamic-keyword-dictionaries)].
However, the implementation of this library is enhanced from the conference version.

The technical details are now being written.

## Implementation overview

This library implements an associative array giving a mapping from key strings to values of any type and supporting dynamic update like `std::map<std::string,V>`.
The data structure is based on a dynamic path-decomposed trie.
The nodes are located on a hash table.
The library implements the hash table of the two classes:

- `HashTriePR` is a plain representation of a hash table.
- `HashTrieCR` is a compact representation of a hash table based on [m-Bonsai](https://arxiv.org/abs/1704.05682).

And the trie has string labels for each node, so their pointers have to be stored.
The library includes the three management methods:

- `LabelStorePM` simply stores all pointers to string labels.
- `LabelStoreEM` embeds short string labels into spaces of pointers.
- `LabelStoreGM` reduces the overhead by grouping pointers in the same manner as [sparsehash](https://github.com/sparsehash/sparsehash).

Class `Map` implements the associative array and takes `HashTrie*` and `LabelStore*` as the template arguments.
That is to say, there are implementations of six classes.
But, you can easily get the implementations since `poplar.hpp` provides the following aliases:

- `MapPP` = `Map` + `HashTriePR` + `LabelStorePM` (fastest)
- `MapPE` = `Map` + `HashTriePR` + `LabelStoreEM`
- `MapPG` = `Map` + `HashTriePR` + `LabelStoreGM`
- `MapCP` = `Map` + `HashTrieCR` + `LabelStorePM`
- `MapCE` = `Map` + `HashTrieCR` + `LabelStoreEM`
- `MapCG` = `Map` + `HashTrieCR` + `LabelStoreGM` (smallest)



## Build instructions

You can download and compile Poplar-trie as the following commands.

```
$ git clone https://github.com/kampersanda/poplar-trie.git
$ cd poplar-trie
$ mkdir build
$ cd build
$ cmake .. -DPOPLAR_USE_POPCNT=ON
$ make
$ make install
```

This library needs C++17, so please install g++ 7.0 (or greater) or clang 4.0 (or greater).
As can be seen in the above commands, CMake 3.8 (or greater) has to be installed to compile the library.
You can use the SSE4.2 POPCNT instruction by adding `-DPOPLAR_USE_POPCNT=ON`.

## Easy example

The following code is an easy example of inserting and searching key-value pairs.

```c++
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
# of keys is 9
```

## Benchmarks

The main advantage of Poplar-trie is high space efficiency as can be seen in the following results.

The experiments were carried out on Intel Xeon E5 @3.5 GHz CPU, with 32 GB of RAM, running Mac OS X 10.12.
The codes were compiled using Apple LLVM version 8 (clang-8) with optimization -O3.
The dictionaries were constructed by inserting all page titles from Japanese Wikipedia (32.3 MiB) in random order.
The value type is `int`.
The maximum resident set size during construction was measured using the `/usr/bin/time` command.
The insertion time was also measured using `std::chrono::duration_cast`.
And, search time for the same strings was measured.

| Implementation | Space (MiB) | Insertion (micros / key) | Search (micros / key) |
|----------------|------------:|----------------------------:|-------------------------:|
| `MapPP` | 80.4 | **0.68** | 0.48 |
| `MapPE` | 75.6 | 0.91 | 0.57 |
| `MapPG` | 47.2 | 1.71 | 0.80 |
| `MapCP` | 65.5 | 0.81 | 0.54 |
| `MapCE` | 61.6 | 1.00 | 0.61 |
| `MapCG` | **42.3** | 1.62 | 0.85 |
| [JudySL](http://judy.sourceforge.net) | 72.7 | 0.73 | 0.49 |
| [hat-trie](https://github.com/dcjones/hat-trie) | 74.5 | 0.97 | **0.25** |
| [cedarpp](http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/cedar/) | 94.7 | 0.69 | 0.42 |

## Todo

- Support the deletion operation
- Add comments to the codes
- Create the API document

## Related software

- [compact\_sparse\_hash](https://github.com/tudocomp/compact_sparse_hash) is an implementation of a compact associative array with integer keys.
- [mBonsai](https://github.com/Poyias/mBonsai) is the original implementation of succinct dynamic tries.
- [tudocomp](https://github.com/tudocomp/tudocomp) includes many dynamic trie implementations for LZ factorization.
 
## Special thanks

Thanks to [Dr. Dominik Köppl](https://github.com/koeppl) I was able to create the bijective hash function in `bijective_hash.hpp`.

