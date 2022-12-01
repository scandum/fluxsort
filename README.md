Intro
-----
This document describes a hybrid mergesort / quicksort named fluxsort. The sort is stable, adaptive, branchless, and has exceptional performance. A [visualisation](https://github.com/scandum/fluxsort#visualization) and [benchmarks](https://github.com/scandum/fluxsort#benchmarks) are available at the bottom.

Analyzer
--------
Fluxsort starts out with an analyzer that detects fully sorted arrays and sorts reverse order arrays using n comparisons. It also obtains a measure of presortedness and switches to [quadsort](https://github.com/scandum/quadsort) if the array is less than 50% random.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Fluxsort obtains the pseudomedian of 9 for partitions smaller than 1024 elements, and the pseudomedian of 25 otherwise. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

After obtaining a pivot the array is parsed from start to end. Elements smaller than the pivot are copied in-place to the start of the array, elements greater than the pivot are copied to swap memory. The partitioning routine is called recursively on the two partitions in main and swap memory.

Recursively partitioning through both swap and main memory is accomplished by passing along a pointer (ptx) to either swap or main memory, so swap memory does not need to be copied back to main memory before it can be partitioned again.

Worst case handling
-------------------
To avoid run-away recursion fluxsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 4 million for the pseudomedian of 25.

Combined with the analyzer fluxsort starts out with this makes the existence of killer patterns unlikely, other than a 33% performance slowdown by prematurely triggering the use of quadsort.

Branchless optimizations
------------------------
Fluxsort uses a branchless comparison optimization. The ability of quicksort to partition branchless was first described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss. Since Fluxsort uses auxiliary memory the partitioning scheme is simpler and faster than the one used by BlockQuicksort.

Median selection uses a branchless comparison technique that selects the pseudomedian of 9 using 12 comparisons, and the pseudomedian of 25 using 42 comparisons.

These optimizations do not work as well when the comparisons themselves are branched and the largest performance increase is on 32 and 64 bit integers.

Generic data optimizations
--------------------------
Fluxsort uses a method popularized by [pdqsort](https://github.com/orlp/pdqsort) to improve generic data handling. If the same pivot is chosen twice in a row it performs a reverse partition, filtering out all elements equal to the pivot, next it carries on as usual. In addition, if after a partition all elements were smaller or equal to the pivot, a reverse partition is performed. This typically only occurs when sorting tables with many repeating values, like gender, education level, birthyear, zipcode, etc.

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
For partitions larger than 65536 elements fluxsort obtains the median of 128 or 256. It does so by copying 128 or 256 random elements to swap memory, sorting them with quadsort, and taking the center element. Using pseudomedian instead of median selection on large arrays is slower, likely due to cache pollution.

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

Memory
------
Fluxsort allocates n elements of swap memory, which is shared with quadsort. Recursion requires log n stack memory.

If memory allocation fails fluxsort defaults to quadsort, which requires n / 4 elements of swap memory. If allocation fails again quadsort will sort in-place through rotations.

Performance
-----------
Fluxsort is the fastest stable comparison sort written to date.

To take full advantage of branchless operations the `cmp` macro needs to be uncommented in bench.c, which will double the performance on primitive types.

Porting
-------
People wanting to port fluxsort might want to have a look at [twinsort](https://github.com/scandum/twinsort), which is a simplified implementation of quadsort. Fluxsort itself is relatively simple.

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
|stablesort |   100000 |   64 | 0.006106 | 0.006165 |         1 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.001916 | 0.001938 |         1 |     100 |     random order |
|   timsort |   100000 |   64 | 0.007624 | 0.007680 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   32 | 0.006154 | 0.006187 |         1 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001830 | 0.001852 |         1 |     100 |     random order |
|   timsort |   100000 |   32 | 0.007637 | 0.007683 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.004002 | 0.004041 |         1 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000677 | 0.000699 |         1 |     100 |     random % 100 |
|   timsort |   100000 |   32 | 0.005627 | 0.005701 |         1 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000799 | 0.000832 |         1 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000045 |         1 |     100 |  ascending order |
|   timsort |   100000 |   32 | 0.000045 | 0.000045 |         1 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001497 | 0.001524 |         1 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000812 | 0.000820 |         1 |     100 |    ascending saw |
|   timsort |   100000 |   32 | 0.000834 | 0.000840 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000889 | 0.000913 |         1 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000377 | 0.000379 |         1 |     100 |       pipe organ |
|   timsort |   100000 |   32 | 0.000168 | 0.000171 |         1 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000927 | 0.000953 |         1 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000055 | 0.000056 |         1 |     100 | descending order |
|   timsort |   100000 |   32 | 0.000088 | 0.000092 |         1 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001498 | 0.001526 |         1 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000814 | 0.000823 |         1 |     100 |   descending saw |
|   timsort |   100000 |   32 | 0.000832 | 0.000841 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.002158 | 0.002184 |         1 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.000845 | 0.000854 |         1 |     100 |      random tail |
|   timsort |   100000 |   32 | 0.002010 | 0.002033 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003652 | 0.003676 |         1 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001160 | 0.001169 |         1 |     100 |      random half |
|   timsort |   100000 |   32 | 0.004041 | 0.004078 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001092 | 0.001124 |         1 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000333 | 0.000336 |         1 |     100 |  ascending tiles |
|   timsort |   100000 |   32 | 0.000964 | 0.001025 |         1 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001762 | 0.001899 |         1 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.001664 | 0.001687 |         1 |     100 |     bit reversal |
|   timsort |   100000 |   32 | 0.002254 | 0.002745 |         1 |     100 |     bit reversal |

</details>

The following benchmark was on WSL gcc version 7.4.0 (Ubuntu 7.4.0-1ubuntu1~18.04.1).
The source code was compiled using gcc -O3 bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and qsort are not inlined. The stdlib qsort() in the benchmark is a mergesort variant. 

![fluxsort vs qsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_qsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.016893 | 0.017751 |   1536381 |     100 |    random string |
|  fluxsort |   100000 |   64 | 0.010192 | 0.010726 |   1884907 |     100 |    random string |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |  128 | 0.018242 | 0.019490 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.011403 | 0.011871 |   1884438 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.009503 | 0.009730 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.004176 | 0.004258 |   1884438 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   32 | 0.008824 | 0.009017 |   1536634 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.003597 | 0.003648 |   1892483 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.006820 | 0.006973 |   1532465 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.001460 | 0.001500 |    972114 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002606 | 0.002765 |    815024 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000173 | 0.000177 |     99999 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003379 | 0.003525 |    915020 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.001257 | 0.001278 |    553308 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002672 | 0.002800 |    884462 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000712 | 0.000716 |    428065 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002472 | 0.002632 |    853904 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000168 | 0.000168 |     99999 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003293 | 0.003425 |    953892 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.001260 | 0.001276 |    565476 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.004208 | 0.004596 |   1012073 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.001433 | 0.001501 |    670123 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005965 | 0.006254 |   1200713 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.002344 | 0.002421 |   1076830 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003899 | 0.004497 |   1209200 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.002171 | 0.002221 |    789580 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005194 | 0.005992 |   1553378 |     100 |     bit reversal |
|  fluxsort |   100000 |   32 | 0.003480 | 0.003622 |   1898395 |     100 |     bit reversal |

</details>

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and pdqsort are inlined.

![fluxsort vs pdqsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_pdqsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  fluxsort |   100000 |   64 | 0.001907 | 0.001953 |         1 |     100 |     random order |
|   pdqsort |   100000 |   64 | 0.002681 | 0.002707 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|  fluxsort |   100000 |   32 | 0.001835 | 0.001855 |         1 |     100 |     random order |
|   pdqsort |   100000 |   32 | 0.002685 | 0.002699 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000677 | 0.000692 |         1 |     100 |     random % 100 |
|   pdqsort |   100000 |   32 | 0.000777 | 0.000785 |         1 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000044 | 0.000044 |         1 |     100 |  ascending order |
|   pdqsort |   100000 |   32 | 0.000084 | 0.000086 |         1 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000810 | 0.000831 |         1 |     100 |    ascending saw |
|   pdqsort |   100000 |   32 | 0.003462 | 0.003481 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000377 | 0.000383 |         1 |     100 |       pipe organ |
|   pdqsort |   100000 |   32 | 0.002832 | 0.002903 |         1 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000056 | 0.000057 |         1 |     100 | descending order |
|   pdqsort |   100000 |   32 | 0.000188 | 0.000192 |         1 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000809 | 0.000819 |         1 |     100 |   descending saw |
|   pdqsort |   100000 |   32 | 0.003232 | 0.003258 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000843 | 0.000854 |         1 |     100 |      random tail |
|   pdqsort |   100000 |   32 | 0.002564 | 0.002589 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.001161 | 0.001166 |         1 |     100 |      random half |
|   pdqsort |   100000 |   32 | 0.002666 | 0.002682 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.000334 | 0.000354 |         1 |     100 |  ascending tiles |
|   pdqsort |   100000 |   32 | 0.002316 | 0.002346 |         1 |     100 |  ascending tiles |
|           |          |      |          |          |           |         |                  |
|  fluxsort |   100000 |   32 | 0.001661 | 0.001686 |         1 |     100 |     bit reversal |
|   pdqsort |   100000 |   32 | 0.002664 | 0.002679 |         1 |     100 |     bit reversal |

</details>
