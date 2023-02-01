Intro
-----
This document describes a hybrid mergesort / quicksort named fluxsort. The sort is stable, adaptive, branchless, and has exceptional performance. A [visualisation](https://github.com/scandum/fluxsort#visualization) and [benchmarks](https://github.com/scandum/fluxsort#benchmarks) are available at the bottom.

Analyzer
--------
Fluxsort starts out with an analyzer that detects fully sorted arrays and sorts reverse order arrays using n comparisons. It also splits the array in 4 segments and obtains a measure of presortedness for each segment, switching to [quadsort](https://github.com/scandum/quadsort) if the segment is more than 50% random.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Fluxsort obtains the pseudomedian of 9 for partitions smaller than 2024 elements, the pseudomedian of 25 if the array is smaller than 65536, and the median of 128, 256, or 512 otherwise, depending on array size. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

After obtaining a pivot the array is parsed from start to end. Elements smaller than the pivot are copied in-place to the start of the array, elements greater than the pivot are copied to swap memory. The partitioning routine is called recursively on the two partitions in main and swap memory.

Recursively partitioning through both swap and main memory is accomplished by passing along a pointer (ptx) to either swap or main memory, so swap memory does not need to be copied back to main memory before it can be partitioned again.

Worst case handling
-------------------
To avoid run-away recursion fluxsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 4 million for the pseudomedian of 25.

Combined with the analyzer fluxsort starts out with this makes the existence of killer patterns unlikely, other than at most a 33% performance slowdown by prematurely triggering the use of quadsort.

Branchless optimizations
------------------------
Fluxsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss. Since Fluxsort uses auxiliary memory the partitioning scheme is simpler and faster than the one used by BlockQuicksort.

Median selection uses a branchless comparison technique that selects the pseudomedian of 9 using 12 comparisons, and the pseudomedian of 25 using 42 comparisons.

These optimizations do not work as well when the comparisons themselves are branched and the largest performance increase is on 32 and 64 bit integers.

Generic data optimizations
--------------------------
Fluxsort uses a method similar to dual-pivot quicksort to improve generic data handling. If the same pivot is chosen twice in a row it performs a reverse partition, filtering out all elements equal to the pivot, next it carries on as usual. In addition, if after a partition all elements were smaller or equal to the pivot, a reverse partition is performed. This typically only occurs when sorting tables with many identical values, like gender, birthyear, etc.

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

Adaptive partitioning
---------------------
Fluxsort performs low cost run detection while it partitions and switches to quadsort if a long run is detected. While the run detection is not fully robust it can result in significant performance gains at a neglible cost.

Large array optimizations
-------------------------
For partitions larger than 65536 elements fluxsort obtains the median of 128 or 256. It does so by copying 128 or 256 random elements to swap memory, sorting them with quadsort, and taking the center element.

Small array optimizations
-------------------------
For paritions smaller than 24 elements fluxsort uses quadsort's small array sorting routine. This routine uses branchless parity merges for the first 8 or 16 elements, and twice-unguarded insertion sort to sort the remainder. This gives a significant performance gain compared to the unguarded insertion sort used by most quicksorts.

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

Fluxsort comes with the fluxsort_prim(void *array, size_t nmemb, size_t size) function to perform primitive comparisons on arrays of 32 and 64 bit integers. Nmemb is the number of elements. Size should be either sizeof(int) or sizeof(long long) for signed integers, and sizeof(int) + 1 or sizeof(long long) + 1 for unsigned integers. 

Memory
------
Fluxsort allocates n elements of swap memory, which is shared with quadsort. Recursion requires log n stack memory.

If memory allocation fails fluxsort defaults to quadsort, which requires n / 4 elements of swap memory. If allocation fails again quadsort will sort in-place through rotations.

Performance
-----------
Fluxsort is one of the fastest stable comparison sort written to date.

To take full advantage of branchless operations the `cmp` macro needs to be uncommented in bench.c, which will double the performance on primitive types.

Porting
-------
People wanting to port fluxsort might want to have a look at [piposort](https://github.com/scandum/piposort), which is a simplified implementation of quadsort. Fluxsort itself is relatively simple.

Visualization
-------------
In the visualization below four tests are performed on 512 elements: Random, Generic, Random Half, and Ascending Tiles. Partitions greater than 48 elements use the pseudomedian of 15 to select the pivot.

[![fluxsort visualization](https://github.com/scandum/fluxsort/blob/main/images/fluxsort.gif)](https://youtu.be/pXPrCTi-gRE)

Benchmarks
----------

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for timsort, fluxsort and std::stable_sort are inlined.

![fluxsort vs stdstablesort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_stdstablesort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |  128 | 0.010934 | 0.011043 |         0 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.008522 | 0.008594 |         0 |     100 |     random order |
|   timsort |   100000 |  128 | 0.012931 | 0.013013 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   64 | 0.006097 | 0.006133 |         0 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.001935 | 0.001954 |         0 |     100 |     random order |
|   timsort |   100000 |   64 | 0.007722 | 0.007768 |         0 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   32 | 0.006066 | 0.006097 |         0 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001840 | 0.001858 |         0 |     100 |     random order |
|   timsort |   100000 |   32 | 0.007620 | 0.007651 |         0 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003911 | 0.003933 |         0 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000658 | 0.000671 |         0 |     100 |     random % 100 |
|   timsort |   100000 |   32 | 0.005604 | 0.005629 |         0 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000692 | 0.000725 |         0 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000044 |         0 |     100 |  ascending order |
|   timsort |   100000 |   32 | 0.000070 | 0.000070 |         0 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001372 | 0.001386 |         0 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000329 | 0.000334 |         0 |     100 |    ascending saw |
|   timsort |   100000 |   32 | 0.000874 | 0.000882 |         0 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000832 | 0.000838 |         0 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000215 | 0.000217 |         0 |     100 |       pipe organ |
|   timsort |   100000 |   32 | 0.000183 | 0.000183 |         0 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000914 | 0.000929 |         0 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000055 | 0.000055 |         0 |     100 | descending order |
|   timsort |   100000 |   32 | 0.000088 | 0.000091 |         0 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001376 | 0.001420 |         0 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000328 | 0.000330 |         0 |     100 |   descending saw |
|   timsort |   100000 |   32 | 0.000874 | 0.000882 |         0 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.002064 | 0.002086 |         0 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.000626 | 0.000632 |         0 |     100 |      random tail |
|   timsort |   100000 |   32 | 0.002024 | 0.002035 |         0 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003547 | 0.003587 |         0 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001067 | 0.001073 |         0 |     100 |      random half |
|   timsort |   100000 |   32 | 0.004046 | 0.004063 |         0 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000972 | 0.000988 |         0 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000293 | 0.000295 |         0 |     100 |  ascending tiles |
|   timsort |   100000 |   32 | 0.000885 | 0.000916 |         0 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001522 | 0.001587 |         0 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.001694 | 0.001720 |         0 |     100 |     bit reversal |
|   timsort |   100000 |   32 | 0.002240 | 0.002450 |         0 |     100 |     bit reversal |

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
