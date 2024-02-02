Intro
-----
This document describes a stable quicksort / mergesort hybrid named fluxsort. The sort is stable, adaptive, branchless, and has exceptional performance. A [visualisation](https://github.com/scandum/fluxsort#visualization) and [benchmarks](https://github.com/scandum/fluxsort#benchmarks) are available at the bottom.

Analyzer
--------
Fluxsort starts out with an analyzer that handles fully in-order arrays and reverse-order arrays using n comparisons. It also splits the array in 4 segments and obtains a measure of presortedness for each segment, switching to [quadsort](https://github.com/scandum/quadsort) if the segment is more than 50% ordered.

While not as adaptive as the bottom-up run detection used by quadsort, a top-down analyzer works well because quicksort significantly benefits from sorting longer ranges. This approach results in more robust overall adaptivity as fluxsort cannot be tricked into performing less efficient partitions on small sorted runs. If only random data is found the analyzer starts skipping ahead, only using n / 4 comparisons to analyze random data.

Increasing the segments from 4 to 16 is challenging due to register pressure and code size.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Fluxsort obtains the quasimedian of 9 for partitions smaller than 2024 elements, and the median of 32, 64, 128, 256, 512, or 1024 for larger partitions, making the pivot selection an approximation of the cubic root of the partition size. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 96 elements are sorted with quadsort.

For the quasimedian of 9 I developed a very efficient and elegant branchless median of 3 computation.
```c
        int x = swap[0] > swap[1];
        int y = swap[0] > swap[2];
        int z = swap[1] > swap[2];

        return swap[(x == y) + (y ^ z)];
```

Fluxsort's cubic root median selection differs from traditional pseudomedian (median of medians) selection by utilizing a combination of quadsort and a binary search.

After obtaining a pivot the array is parsed from start to end. Elements equal or smaller than the pivot are copied in-place to the start of the array, elements greater than the pivot are copied to swap memory. The partitioning routine is called recursively on the two partitions in main and swap memory.

The flux partitioning scheme is partially in-place, giving it a performance advantage over mergesort for large arrays.

Worst case handling
-------------------
To avoid run-away recursion fluxsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 3,000 for the quasimedian of 9 and less than 1 in 10 million for the quasimedian of 32.

Combined with cbrt(n) median selection, this guarantees a worst case of `n log n` comparisons.

Branchless optimizations
------------------------
Fluxsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss.[^1] Since Fluxsort uses auxiliary memory, the partitioning scheme is simpler and faster than the one used by BlockQuicksort.

Median selection uses a branchless comparison technique that selects the quasimedian of 9 using 15 comparisons, and the median of 32 using 115 comparisons.

When sorting, branchless comparisons are primarily useful to take advantage of memory-level parallelism. After writing data, the process can continue without having to wait for the write operation to have actually finished, and the process will primarily stall when a cache line is fetched. Since quicksort partitions to two memory regions, part of the loop can continue, reducing the wait time for cache line fetches. This gives an overall performance gain, even though the branchless operation is more expensive due to a lack of support for efficient branchless operations in C / gcc.

When the comparison becomes more expensive (like string comparisons), the size of the type is increased, the size of the partition is increased, or the comparison accesses uncached memory regions, the benefit of memory-level parallelism is reduced, and can even result in slower overall execution. While it's possible to write to four memory regions at once, instead of two, the cost-benefit is dubious, though it might be a good strategy for future hardware.

Quadsort, as of September 2021, uses a branchless optimization as well, and writes to two distinct memory regions by merging both ends of an array simultaneously. For sorting strings and objects quadsort's overall branchless performance is better than fluxsort's, with the exception that fluxsort is faster on random data with low cardinality.

As a general note, branch prediction is awesome. Quadsort and fluxsort try to take advantage of branch prediction where possible.

Generic data optimizations
--------------------------
Fluxsort uses a method that mimicks dual-pivot quicksort to improve generic data handling. If after a partition all elements were smaller or equal to the pivot, a second sweep is performed, filtering out all elements equal to the pivot, next it carries on as usual. This typically only occurs when sorting tables with many identical values, like gender, age, etc. Generic data performance is improved slightly by checking if the same pivot is chosen twice in a row, in which case it performs a reverse partition as well. To my knowledge, pivot retention was first introduced by [pdqsort](https://github.com/orlp/pdqsort). Generic data performance is further improved by defaulting to quadsort in some cases.

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
This diagram and the 'second sweep' quicksort optimization for equal keys was described as early as 1985 by Lutz M Wegner.[^2]

Since equal elements are copied back to the input array it is guaranteed that no more than n - 3 elements are copied to swap memory. Subsequently fluxsort can store a stack of previously used pivots at the end of swap memory.

Adaptive partitioning
---------------------
Fluxsort performs low cost run detection while it partitions and switches to quadsort if a potential long run is detected. While the run detection is not fully robust it can result in significant performance gains at a neglible cost.

Large array optimizations
-------------------------
For partitions larger than 2048 elements fluxsort obtains the median of 32, 64, 128, 256, 512, or 1024. It does so by copying random elements to swap memory, sorting two halves with quadsort, and returning the center right element using a binary search.

Small array optimizations
-------------------------
For partitions smaller than 96 elements fluxsort uses quadsort's small array sorting routine. This routine uses branchless parity merges for the first 4, 8 or 16 elements, and twice-unguarded insertion sort to sort the remainder. If the array exceeds 24 elements, it is split in 4 segments, and parity merged. This gives a significant performance gain compared to the unguarded insertion sort used by most introsorts.

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

Fluxsort comes with the fluxsort_size(void *array, size_t nmemb, size_t size, CMPFUNC *cmp) function to sort elements of any given size. The comparison function needs to be by reference, instead of by value, as if you are sorting an array of pointers.

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

Credits
-------
I was likely the first to write a branchless stable quicksort and I introduced a few new things, like branchless pivot selection, increasing the pivot candidate selection from the pseudomedian of 9 to an approximation of sqrt(n), improving small partition sorting by utilizing branchless parity merges, stably partitioning partially in-place, run detection for increased adaptivity, detecting emergent patterns, and an efficient switch between partitioning swap and main memory.

Special thanks to Control from the Holy Grail Sort Project for invaluable feedback and insights.

Visualization
-------------
In the visualization below eleven tests are performed on 256 elements. The various distributions are indexed on YouTube: https://youtu.be/TDJ5zpcZJ18.

[![fluxsort visualization](/images/fluxsort.gif)](https://youtu.be/TDJ5zpcZJ18)

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

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  quadsort |   131072 |   32 | 0.002174 | 0.002209 |         0 |     100 |     random order |
|  fluxsort |   131072 |   32 | 0.002189 | 0.002205 |         0 |     100 |     random order |
| glidesort |   131072 |   32 | 0.003065 | 0.003125 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001623 | 0.001646 |         0 |     100 |     random % 100 |
|  fluxsort |   131072 |   32 | 0.000837 | 0.000856 |         0 |     100 |     random % 100 |
| glidesort |   131072 |   32 | 0.001031 | 0.001037 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000061 | 0.000063 |         0 |     100 |  ascending order |
|  fluxsort |   131072 |   32 | 0.000058 | 0.000060 |         0 |     100 |  ascending order |
| glidesort |   131072 |   32 | 0.000091 | 0.000093 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000345 | 0.000353 |         0 |     100 |    ascending saw |
|  fluxsort |   131072 |   32 | 0.000341 | 0.000349 |         0 |     100 |    ascending saw |
| glidesort |   131072 |   32 | 0.000351 | 0.000358 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000231 | 0.000245 |         0 |     100 |       pipe organ |
|  fluxsort |   131072 |   32 | 0.000222 | 0.000228 |         0 |     100 |       pipe organ |
| glidesort |   131072 |   32 | 0.000228 | 0.000235 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000074 | 0.000076 |         0 |     100 | descending order |
|  fluxsort |   131072 |   32 | 0.000073 | 0.000076 |         0 |     100 | descending order |
| glidesort |   131072 |   32 | 0.000106 | 0.000110 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000373 | 0.000380 |         0 |     100 |   descending saw |
|  fluxsort |   131072 |   32 | 0.000355 | 0.000371 |         0 |     100 |   descending saw |
| glidesort |   131072 |   32 | 0.000363 | 0.000369 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.000685 | 0.000697 |         0 |     100 |      random tail |
|  fluxsort |   131072 |   32 | 0.000720 | 0.000726 |         0 |     100 |      random tail |
| glidesort |   131072 |   32 | 0.000953 | 0.000966 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001192 | 0.001204 |         0 |     100 |      random half |
|  fluxsort |   131072 |   32 | 0.001251 | 0.001266 |         0 |     100 |      random half |
| glidesort |   131072 |   32 | 0.001650 | 0.001679 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001472 | 0.001507 |         0 |     100 |  ascending tiles |
|  fluxsort |   131072 |   32 | 0.000578 | 0.000589 |         0 |     100 |  ascending tiles |
| glidesort |   131072 |   32 | 0.002559 | 0.002576 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.002210 | 0.002231 |         0 |     100 |     bit reversal |
|  fluxsort |   131072 |   32 | 0.002042 | 0.002053 |         0 |     100 |     bit reversal |
| glidesort |   131072 |   32 | 0.002787 | 0.002807 |         0 |     100 |     bit reversal |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001237 | 0.001278 |         0 |     100 |       random % 2 |
|  fluxsort |   131072 |   32 | 0.000227 | 0.000233 |         0 |     100 |       random % 2 |
| glidesort |   131072 |   32 | 0.000449 | 0.000455 |         0 |     100 |       random % 2 |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001123 | 0.001153 |         0 |     100 |           signal |
|  fluxsort |   131072 |   32 | 0.001269 | 0.001285 |         0 |     100 |           signal |
| glidesort |   131072 |   32 | 0.003760 | 0.003776 |         0 |     100 |           signal |
|           |          |      |          |          |           |         |                  |
|  quadsort |   131072 |   32 | 0.001911 | 0.001956 |         0 |     100 |      exponential |
|  fluxsort |   131072 |   32 | 0.001134 | 0.001142 |         0 |     100 |      exponential |
| glidesort |   131072 |   32 | 0.002355 | 0.002373 |         0 |     100 |      exponential |

</details>

Footnotes
---------
[^1]: [BlockQuicksort: How Branch Mispredictions don't affect Quicksort, Stefan Edelkamp and Armin Weiß, April 2016](https://arxiv.org/pdf/1604.06697.pdf)
[^2]: [Quicksort for Equal Keys, Lutz M. Wegner, April 1985](https://www.researchgate.net/publication/220329488_Quicksort_for_Equal_Keys)
