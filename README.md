Intro
-----
This document describes a stable quicksort / mergesort hybrid named fluxsort. The sort is stable, adaptive, branchless, and has exceptional performance. A [visualisation](https://github.com/scandum/fluxsort#visualization) and [benchmarks](https://github.com/scandum/fluxsort#benchmarks) are available at the bottom.

Analyzer
--------
Fluxsort starts out with an analyzer that handles fully in-order arrays and reverse-order arrays using n comparisons. It also splits the array in 4 segments and obtains a measure of presortedness for each segment, switching to [quadsort](https://github.com/scandum/quadsort) if the segment is more than 50% ordered.

While arguably not as adaptive as the bottom-up analyzer used by quadsort, a top-down analyzer works well because quicksort significantly benefits from sorting longer ranges. This approach results in more robust overall adaptivity as fluxsort cannot be tricked into performing less efficient partitions by small sorted runs. In addition, the analyzer executes what could be considered a partial top-down merge on the 4 segments.

Increasing the segments from 4 to 16 is challenging due to register pressure.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Fluxsort obtains the pseudomedian of 9 for partitions smaller than 2024 elements, the pseudomedian of 25 if the array is smaller than 65536, and the median of 128, 256, or 512 for larger partitions, making the pivot selection an approximation of the cubic root of the partition size. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

After obtaining a pivot the array is parsed from start to end. Elements smaller than the pivot are copied in-place to the start of the array, elements greater than the pivot are copied to swap memory. The partitioning routine is called recursively on the two partitions in main and swap memory.

Recursively partitioning through both swap and main memory is accomplished by passing along a pointer (ptx) to either swap or main memory, so swap memory does not need to be copied back to main memory before it can be partitioned again.

Worst case handling
-------------------
To avoid run-away recursion fluxsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 4 million for the pseudomedian of 25.

Combined with the analyzer fluxsort starts out with this makes the existence of killer patterns unlikely, other than at most a 33% performance slowdown by prematurely triggering the use of quadsort. However, for more complex comparisons, like strings, quadsort matches fluxsort, in which case it wouldn't make a difference.

Branchless optimizations
------------------------
Fluxsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss. Since Fluxsort uses auxiliary memory the partitioning scheme is simpler and faster than the one used by BlockQuicksort.

Median selection uses a branchless comparison technique that selects the pseudomedian of 9 using 12 comparisons, and the pseudomedian of 25 using 42 comparisons.

When sorting, branchless comparisons are primarily useful to take advantage of memory-level parallelism. After writing data, the process can continue without having to wait for the write operation to have actually finished, and the process will primarily stall when a cache line is fetched. Since quicksort partitions to two memory regions, part of the loop can continue, reducing the wait time for cache line fetches. This gives an overall performance gain, even though the branchless operation is more expensive due to a lack of support for branchless operations in C and gcc.

Using the clang compiler it's possible to create a branchless ternary merge using `*dest++ = (*left <= *right) ? *left++ : *right++` but C doesn't allow for a branchless ternary partition, which would look like: `*left++ : *right++ ? (*from <= *pivot) = *from++`. Subsequently branchless partitioning has to make `n log n` unnecessary moves.

When the comparison becomes more expensive (like string comparisons), the size of the type is increased, the size of the partition is increased, or the comparison accesses uncached memory regions, the benefit of memory-level parallelism is reduced, and can even result in slower overall execution. While it's possible to write to four memory regions at once, instead of two, the cost-benefit is dubious for a general purpose sort, the added complexity would make porting and validating the code less appealing, and the performance gains are hardware dependent.

Quadsort, as of September 2021, uses a branchless optimization as well, and writes to two distinct memory regions by merging both ends of an array simultaneously. For sorting strings and objects quadsort's overall branchless performance is better than fluxsort's with the exception that fluxsort is faster on random data with low cardinality.

As a general note, branch prediction is awesome. Quadsort and fluxsort try to take advantage of branch prediction where possible.

Generic data optimizations
--------------------------
Fluxsort uses a method that mimicks dual-pivot quicksort to improve generic data handling. If after a partition all elements were smaller or equal to the pivot, a reverse partition is performed, filtering out all elements equal to the pivot, next it carries on as usual. This typically only occurs when sorting tables with many identical values, like gender, age, etc. Fluxsort has a small bias in its pivot selection to increase the odds of this happening. In addition, generic data performance is improved slightly by checking if the same pivot is chosen twice in a row, in which case it performs a reverse partition as well. Pivot retention was first introduced by [pdqsort](https://github.com/orlp/pdqsort).

```
┌──────────────────────────────────┬───┬──────────────┐
│             E <= P               │ P │    E > P     | default partition
└──────────────────────────────────┴───┴──────────────┘

┌──────────────┬───┬───────────────────┐
│    P > E     │ P │    P <= E         |                reverse partition
└──────────────┴───┴───────────────────┘

┌──────────────┬───┬───────────────┬───┬──────────────┐
│    E < P     │ P │    E == P     │ P │     E > P    | 
└──────────────┴───┴───────────────┴───┴──────────────┘
```

Since equal elements are copied back to the input array it is guaranteed that no more than n - 3 elements are copied to swap memory. Subsequently fluxsort can store a stack of previously used pivots at the end of swap memory.

Adaptive partitioning
---------------------
Fluxsort performs low cost run detection while it partitions and switches to quadsort if a potential long run is detected. While the run detection is not fully robust it can result in significant performance gains at a neglible cost.

Large array optimizations
-------------------------
For partitions larger than 65536 elements fluxsort obtains the median of 128, 256, or 512. It does so by copying 128, 256, or 512 random elements to swap memory, sorting them with quadsort and taking the center right element.

Small array optimizations
-------------------------
For partitions smaller than 24 elements fluxsort uses quadsort's small array sorting routine. This routine uses branchless parity merges for the first 4 or 8 elements, and twice-unguarded insertion sort to sort the remainder. If the array exceeds 15 elements it is split in 4 segments and parity merged. This gives a significant performance gain compared to the unguarded insertion sort used by most introsorts.

Big O
-----
```
                 ┌───────────────────────┐┌───────────────────────┐
                 │comparisons            ││swap memory            │
┌───────────────┐├───────┬───────┬───────┤├───────┬───────┬───────┤┌──────┐┌─────────┐┌─────────┐
│name           ││min    │avg    │max    ││min    │avg    │max    ││stable││partition││adaptive │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│fluxsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quadsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││no       ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quicksort      ││n log n│n log n│n²     ││1      │1      │1      ││no    ││yes      ││no       │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│pdqsort        ││n      │n log n│n log n││1      │1      │1      ││no    ││yes      ││semi     │
└───────────────┘└───────┴───────┴───────┘└───────┴───────┴───────┘└──────┘└─────────┘└─────────┘
```

Data Types
----------
The C implementation of fluxsort supports long doubles and 8, 16, 32, and 64 bit data types. By using pointers it's possible to sort any other data type, like strings.

Interface
---------
Fluxsort uses the same interface as qsort, which is described in [man qsort](https://man7.org/linux/man-pages/man3/qsort.3p.html).

Fluxsort comes with the fluxsort_prim(void *array, size_t nmemb, size_t size) function to perform primitive comparisons on arrays of 32 and 64 bit integers. Nmemb is the number of elements. Size should be either sizeof(int) or sizeof(long long) for signed integers, and sizeof(int) + 1 or sizeof(long long) + 1 for unsigned integers. Support for additional primitive as well as custom types can be added to fluxsort.h and quadsort.h.

Memory
------
Fluxsort allocates n elements of swap memory, which is shared with quadsort. Recursion requires log n stack memory.

If memory allocation fails fluxsort defaults to quadsort, which can sort in-place through rotations.

If in-place stable sorting is desired the best option is to use [blitsort](https://github.com/scandum/blitsort), which is a properly in-place alternative to fluxsort. For in-place unstable sorting [crumsort](https://github.com/scandum/blitsort) is an option as well.

Performance
-----------
Fluxsort is one of the fastest stable comparison sorts written to date.

To take full advantage of branchless operations the `cmp` macro can be uncommented in bench.c, which will double the performance on primitive types. Fluxsort, after crumsort, is faster than a radix sort for sorting 64 bit integers. An adaptive radix sort, like [wolfsort](https://github.com/scandum/wolfsort), has better performance on 8, 16, and 32 bit types.

Fluxsort needs to be compiled using `gcc -O3` for optimal performance.

Porting
-------
People wanting to port fluxsort might want to have a look at [piposort](https://github.com/scandum/piposort), which is a simplified implementation of quadsort. Fluxsort itself is relatively simple. Earlier versions of fluxsort have a less bulky analyzer. Fluxsort works without the analyzer, but will be less adaptive.

Variants
--------
- [blitsort](https://github.com/scandum/blitsort) is an in-place variant of fluxsort. By default blitsort uses 512 elements of auxiliary memory, but it can easily be used with anywhere from 32 to n elements. It can be configured to use sqrt(n) memory, but other schemes are possible, allowing blitsort to outperform fluxsort by optimizing memory use for a specific system. The only other difference with fluxsort is that it currently does not detect emergent patterns.

- [crumsort](https://github.com/scandum/crumsort) is a hybrid unstable in-place quicksort / quadsort. Crumsort has many similarities with fluxsort, but it uses a novel in-place and unstable partitioning scheme.

- [piposort](https://github.com/scandum/piposort) is a simplified branchless quadsort with a much smaller code size and complexity while still being very fast. Piposort might be of use to people who want to port quadsort. This is a lot easier when you start out small.

- [wolfsort](https://github.com/scandum/wolfsort) is a hybrid stable radixsort / fluxsort with improved performance on random data. It's mostly a proof of concept that only works on unsigned 32 bit integers.

- [glidesort](https://github.com/orlp/glidesort) is a hybrid stable quicksort / timsort written in Rust. The timsort is enhanced with quadsort's bidirectional branchless merge logic. Partitioning is similar to fluxsort, except that it is bidirectional like a parity merge, writing to 4 instead of 2 memory regions. Similarly, the memory regions of the merge routine are increase from 2 to 4 through partitioning and conjoining quad merges. Increasing the memory regions can both increase and decrease performance. Like fluxsort, pivot selection is branchless and pivot candidate selection is an approximation of the square root of the partition size for large arrays. Small array sorting is sped up by using quadsort's branchless parity merges.

Visualization
-------------
In the visualization below four tests are performed on 512 elements: Random, Generic, Random Half, and Ascending Tiles. Partitions greater than 48 elements use the pseudomedian of 15 to select the pivot.

[![fluxsort visualization](/images/fluxsort.gif)](https://youtu.be/Zg7OkQxmohg)

Benchmarks
----------

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for timsort, fluxsort and std::stable_sort are inlined.

![fluxsort vs stdstablesort](/images/fluxsort_vs_stdstablesort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   64 | 0.006008 | 0.006093 |         0 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.002032 | 0.002072 |         0 |     100 |     random order |
|   timsort |   100000 |   64 | 0.007754 | 0.007814 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   32 | 0.006113 | 0.006152 |         0 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001906 | 0.001916 |         0 |     100 |     random order |
|   timsort |   100000 |   32 | 0.007630 | 0.007696 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003954 | 0.003988 |         0 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000677 | 0.000683 |         0 |     100 |     random % 100 |
|   timsort |   100000 |   32 | 0.005648 | 0.005688 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000804 | 0.000859 |         0 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000044 |         0 |     100 |  ascending order |
|   timsort |   100000 |   32 | 0.000045 | 0.000045 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001496 | 0.001548 |         0 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000305 | 0.000307 |         0 |     100 |    ascending saw |
|   timsort |   100000 |   32 | 0.000846 | 0.000852 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001206 | 0.001226 |         0 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000193 | 0.000195 |         0 |     100 |       pipe organ |
|   timsort |   100000 |   32 | 0.000471 | 0.000477 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000914 | 0.000924 |         0 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000056 | 0.000056 |         0 |     100 | descending order |
|   timsort |   100000 |   32 | 0.000102 | 0.000102 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001622 | 0.001644 |         0 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000315 | 0.000316 |         0 |     100 |   descending saw |
|   timsort |   100000 |   32 | 0.000908 | 0.000922 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.002154 | 0.002215 |         0 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.000641 | 0.000646 |         0 |     100 |      random tail |
|   timsort |   100000 |   32 | 0.002005 | 0.002033 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003633 | 0.003678 |         0 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001096 | 0.001108 |         0 |     100 |      random half |
|   timsort |   100000 |   32 | 0.004042 | 0.004072 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001091 | 0.001121 |         0 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000293 | 0.000294 |         0 |     100 |  ascending tiles |
|   timsort |   100000 |   32 | 0.000945 | 0.001002 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001747 | 0.001887 |         0 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.001705 | 0.001718 |         0 |     100 |     bit reversal |
|   timsort |   100000 |   32 | 0.002282 | 0.002920 |         0 |     100 |     bit reversal |

</details>

The following benchmark was on WSL 2 gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)
using the [wolfsort benchmark](https://github.com/scandum/wolfsort).
The source code was compiled using `g++ -O3 -w -fpermissive bench.c`. It measures the performance
on random data with array sizes ranging from 10 to 10,000,000. It's generated by running the benchmark
using 10000000 0 0 as the argument. The benchmark is weighted, meaning the number of repetitions halves
each time the number of items doubles. A table with the best and average time in seconds can be
uncollapsed below the bar graph.

![fluxsort vs stdstablesort](/images/fluxsort_vs_stdstablesort_2.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |       10 |   32 | 0.127779 | 0.128149 |       0.0 |      10 |        random 10 |
|  fluxsort |       10 |   32 | 0.049679 | 0.049883 |       0.0 |      10 |        random 10 |
|   timsort |       10 |   32 | 0.141079 | 0.144350 |       0.0 |      10 |        random 10 |
|           |          |      |          |          |           |         |                  |
|stablesort |      100 |   32 | 0.242744 | 0.243288 |       0.0 |      10 |       random 100 |
|  fluxsort |      100 |   32 | 0.112359 | 0.113083 |       0.0 |      10 |       random 100 |
|   timsort |      100 |   32 | 0.341105 | 0.341710 |       0.0 |      10 |       random 100 |
|           |          |      |          |          |           |         |                  |
|stablesort |     1000 |   32 | 0.362902 | 0.363586 |       0.0 |      10 |      random 1000 |
|  fluxsort |     1000 |   32 | 0.137569 | 0.138066 |       0.0 |      10 |      random 1000 |
|   timsort |     1000 |   32 | 0.493047 | 0.493622 |       0.0 |      10 |      random 1000 |
|           |          |      |          |          |           |         |                  |
|stablesort |    10000 |   32 | 0.476859 | 0.477168 |       0.0 |      10 |     random 10000 |
|  fluxsort |    10000 |   32 | 0.160565 | 0.160774 |       0.0 |      10 |     random 10000 |
|   timsort |    10000 |   32 | 0.637224 | 0.641950 |       0.0 |      10 |     random 10000 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.602276 | 0.602838 |       0.0 |      10 |    random 100000 |
|  fluxsort |   100000 |   32 | 0.187420 | 0.187915 |       0.0 |      10 |    random 100000 |
|   timsort |   100000 |   32 | 0.762078 | 0.762648 |       0.0 |      10 |    random 100000 |
|           |          |      |          |          |           |         |                  |
|stablesort |  1000000 |   32 | 0.731715 | 0.734021 |       0.0 |      10 |   random 1000000 |
|  fluxsort |  1000000 |   32 | 0.217196 | 0.219207 |       0.0 |      10 |   random 1000000 |
|   timsort |  1000000 |   32 | 0.895532 | 0.896547 |       0.0 |      10 |   random 1000000 |
|           |          |      |          |          |           |         |                  |
|stablesort | 10000000 |   32 | 0.891028 | 0.895325 |         0 |      10 |  random 10000000 |
|  fluxsort | 10000000 |   32 | 0.266910 | 0.269159 |         0 |      10 |  random 10000000 |
|   timsort | 10000000 |   32 | 1.081709 | 1.089263 |         0 |      10 |  random 10000000 |

</details>

The following benchmark was on WSL gcc version 7.4.0 (Ubuntu 7.4.0-1ubuntu1~18.04.1).
The source code was compiled using gcc -O3 bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for qsort, fluxsort and quadsort are not inlined. The stdlib qsort() in the benchmark is a mergesort variant. 

![fluxsort vs qsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_qsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.017033 | 0.017227 |   1536381 |     100 |    random string |
|  fluxsort |   100000 |   64 | 0.009928 | 0.010212 |   1782460 |     100 |    random string |
|  quadsort |   100000 |   64 | 0.010777 | 0.010909 |   1684673 |     100 |    random string |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   64 | 0.015398 | 0.015551 |   1536491 |     100 |    random double |
|  fluxsort |   100000 |   64 | 0.007646 | 0.007811 |   1781640 |     100 |    random double |
|  quadsort |   100000 |   64 | 0.008702 | 0.008818 |   1684633 |     100 |    random double |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   64 | 0.011014 | 0.011124 |   1536491 |     100 |      random long |
|  fluxsort |   100000 |   64 | 0.004859 | 0.004965 |   1781640 |     100 |      random long |
|  quadsort |   100000 |   64 | 0.006043 | 0.006120 |   1684633 |     100 |      random long |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   64 | 0.010834 | 0.010965 |   1536634 |     100 |       random int |
|  fluxsort |   100000 |   64 | 0.004596 | 0.004662 |   1790032 |     100 |       random int |
|  quadsort |   100000 |   64 | 0.005365 | 0.005422 |   1684734 |     100 |       random int |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |  128 | 0.018219 | 0.019014 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.011178 | 0.011290 |   1781640 |     100 |     random order |
|  quadsort |   100000 |  128 | 0.011141 | 0.011258 |   1684633 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.009328 | 0.009505 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.004025 | 0.004095 |   1781640 |     100 |     random order |
|  quadsort |   100000 |   64 | 0.004103 | 0.004143 |   1684633 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   32 | 0.008932 | 0.009094 |   1536634 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.003403 | 0.003453 |   1790032 |     100 |     random order |
|  quadsort |   100000 |   32 | 0.003373 | 0.003419 |   1684734 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.006692 | 0.006831 |   1532465 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.001322 | 0.001352 |    897246 |     100 |     random % 100 |
|  quadsort |   100000 |   32 | 0.002723 | 0.002816 |   1415417 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002233 | 0.002371 |    815024 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000174 | 0.000175 |     99999 |     100 |  ascending order |
|  quadsort |   100000 |   32 | 0.000159 | 0.000161 |     99999 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003067 | 0.003177 |    915020 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000545 | 0.000549 |    300011 |     100 |    ascending saw |
|  quadsort |   100000 |   32 | 0.000897 | 0.000915 |    379624 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002480 | 0.002523 |    884462 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000381 | 0.000382 |    200006 |     100 |       pipe organ |
|  quadsort |   100000 |   32 | 0.000457 | 0.000462 |    277113 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002459 | 0.002535 |    853904 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000186 | 0.000187 |     99999 |     100 | descending order |
|  quadsort |   100000 |   32 | 0.000164 | 0.000166 |     99999 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003289 | 0.003361 |    953892 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000555 | 0.000561 |    300013 |     100 |   descending saw |
|  quadsort |   100000 |   32 | 0.000922 | 0.000930 |    391547 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003943 | 0.004005 |   1012073 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.001181 | 0.001207 |    623604 |     100 |      random tail |
|  quadsort |   100000 |   32 | 0.001203 | 0.001221 |    592061 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005791 | 0.005986 |   1200713 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001974 | 0.002004 |   1028725 |     100 |      random half |
|  quadsort |   100000 |   32 | 0.002055 | 0.002092 |   1006728 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.004085 | 0.004223 |   1209200 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.001523 | 0.001557 |    528889 |     100 |  ascending tiles |
|  quadsort |   100000 |   32 | 0.002073 | 0.002130 |    671244 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005143 | 0.005429 |   1553378 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.003263 | 0.003385 |   1798806 |     100 |     bit reversal |
|  quadsort |   100000 |   32 | 0.003179 | 0.003218 |   1727134 |     100 |     bit reversal |

</details>

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for pdqsort, fluxsort and crumsort are inlined.

![fluxsort vs pdqsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_pdqsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |  128 | 0.005854 | 0.005954 |         0 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.008555 | 0.008662 |         0 |     100 |     random order |
|  crumsort |   100000 |  128 | 0.008253 | 0.008312 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   64 | 0.002660 | 0.002683 |         0 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.001950 | 0.001973 |         0 |     100 |     random order |
|  crumsort |   100000 |   64 | 0.001850 | 0.001864 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   32 | 0.002687 | 0.002711 |         0 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001826 | 0.001857 |         0 |     100 |     random order |
|  crumsort |   100000 |   32 | 0.001799 | 0.001815 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000785 | 0.000792 |         0 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000656 | 0.000669 |         0 |     100 |     random % 100 |
|  crumsort |   100000 |   32 | 0.000560 | 0.000565 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000091 | 0.000091 |         0 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000044 |         0 |     100 |  ascending order |
|  crumsort |   100000 |   32 | 0.000044 | 0.000044 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.003465 | 0.003482 |         0 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000329 | 0.000337 |         0 |     100 |    ascending saw |
|  crumsort |   100000 |   32 | 0.000628 | 0.000636 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002840 | 0.002862 |         0 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000215 | 0.000218 |         0 |     100 |       pipe organ |
|  crumsort |   100000 |   32 | 0.000359 | 0.000364 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000195 | 0.000200 |         0 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000055 | 0.000056 |         0 |     100 | descending order |
|  crumsort |   100000 |   32 | 0.000056 | 0.000056 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.003236 | 0.003275 |         0 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000329 | 0.000331 |         0 |     100 |   descending saw |
|  crumsort |   100000 |   32 | 0.000637 | 0.000648 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002566 | 0.002587 |         0 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.000624 | 0.000629 |         0 |     100 |      random tail |
|  crumsort |   100000 |   32 | 0.000879 | 0.000888 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002670 | 0.002697 |         0 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001064 | 0.001081 |         0 |     100 |      random half |
|  crumsort |   100000 |   32 | 0.001199 | 0.001216 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002316 | 0.002425 |         0 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000293 | 0.000299 |         0 |     100 |  ascending tiles |
|  crumsort |   100000 |   32 | 0.001518 | 0.001545 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002670 | 0.002694 |         0 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.001692 | 0.001737 |         0 |     100 |     bit reversal |
|  crumsort |   100000 |   32 | 0.001785 | 0.001805 |         0 |     100 |     bit reversal |

</details>

The following benchmark was on WSL clang version 10 (10.0.0-4ubuntu1~18.04.2) using [rhsort](https://github.com/mlochbaum/rhsort)'s wolfsort benchmark.
The source code was compiled using clang -O3. The bar graph shows the best run out of 100 on 131,072 32 bit integers. Comparisons for quadsort, fluxsort and glidesort are inlined.

Some additional context is required for this benchmark. Glidesort is written and compiled in Rust which supports branchless ternary operations, subsequently fluxsort and quadsort are compiled using clang with branchless ternary operations in place for the merge and small-sort routines. Since fluxsort and quadsort are optimized for gcc there is a performance penalty, with some of the routines running 2-3x slower than they do in gcc.

![fluxsort vs glidesort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_glidesort.png)

<details><summary>data table</summary>

|  quadsort |   131072 |   32 | 0.002134 | 0.002152 |         0 |     100 |     random order |
|  fluxsort |   131072 |   32 | 0.002464 | 0.002502 |         0 |     100 |     random order |
| glidesort |   131072 |   32 | 0.002999 | 0.003017 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001709 | 0.001733 |         0 |     100 |     random % 100 |
|  fluxsort |   131072 |   32 | 0.000902 | 0.000908 |         0 |     100 |     random % 100 |
| glidesort |   131072 |   32 | 0.001011 | 0.001035 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000061 | 0.000062 |         0 |     100 |  ascending order |
|  fluxsort |   131072 |   32 | 0.000058 | 0.000059 |         0 |     100 |  ascending order |
| glidesort |   131072 |   32 | 0.000091 | 0.000092 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000335 | 0.000349 |         0 |     100 |    ascending saw |
|  fluxsort |   131072 |   32 | 0.000334 | 0.000339 |         0 |     100 |    ascending saw |
| glidesort |   131072 |   32 | 0.000346 | 0.000356 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000231 | 0.000242 |         0 |     100 |       pipe organ |
|  fluxsort |   131072 |   32 | 0.000222 | 0.000229 |         0 |     100 |       pipe organ |
| glidesort |   131072 |   32 | 0.000229 | 0.000239 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000073 | 0.000081 |         0 |     100 | descending order |
|  fluxsort |   131072 |   32 | 0.000073 | 0.000082 |         0 |     100 | descending order |
| glidesort |   131072 |   32 | 0.000105 | 0.000109 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000366 | 0.000369 |         0 |     100 |   descending saw |
|  fluxsort |   131072 |   32 | 0.000348 | 0.000354 |         0 |     100 |   descending saw |
| glidesort |   131072 |   32 | 0.000357 | 0.000361 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000687 | 0.000702 |         0 |     100 |      random tail |
|  fluxsort |   131072 |   32 | 0.000792 | 0.000819 |         0 |     100 |      random tail |
| glidesort |   131072 |   32 | 0.000939 | 0.000970 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001177 | 0.001200 |         0 |     100 |      random half |
|  fluxsort |   131072 |   32 | 0.001384 | 0.001401 |         0 |     100 |      random half |
| glidesort |   131072 |   32 | 0.001625 | 0.001652 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001643 | 0.001686 |         0 |     100 |  ascending tiles |
|  fluxsort |   131072 |   32 | 0.000579 | 0.000590 |         0 |     100 |  ascending tiles |
| glidesort |   131072 |   32 | 0.002516 | 0.002543 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.002184 | 0.002199 |         0 |     100 |     bit reversal |
|  fluxsort |   131072 |   32 | 0.002223 | 0.002257 |         0 |     100 |     bit reversal |
| glidesort |   131072 |   32 | 0.002735 | 0.002765 |         0 |     100 |     bit reversal |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001456 | 0.001474 |         0 |     100 |       random % 2 |
|  fluxsort |   131072 |   32 | 0.000359 | 0.000364 |         0 |     100 |       random % 2 |
| glidesort |   131072 |   32 | 0.000443 | 0.000464 |         0 |     100 |       random % 2 |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001332 | 0.001362 |         0 |     100 |           signal |
|  fluxsort |   131072 |   32 | 0.001587 | 0.001602 |         0 |     100 |           signal |
| glidesort |   131072 |   32 | 0.003688 | 0.003711 |         0 |     100 |           signal |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001923 | 0.001947 |         0 |     100 |      exponential |
|  fluxsort |   131072 |   32 | 0.001281 | 0.001291 |         0 |     100 |      exponential |
| glidesort |   131072 |   32 | 0.002313 | 0.002335 |         0 |     100 |      exponential |

</details>
