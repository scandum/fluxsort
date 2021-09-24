Intro
-----
This document describes a hybrid mergesort / quicksort named fluxsort. The sort is stable, adaptive, branchless, and has exceptional performance. A [visualisation](https://github.com/scandum/fluxsort#visualization) and [benchmarks](https://github.com/scandum/fluxsort#benchmarks) are available at the bottom.

Analyzer
--------
Fluxsort starts out with an analyzer that detects fully sorted arrays and sorts reverse order arrays using n comparisons. It also obtains a measure of presortedness and switches to [quadsort](https://github.com/scandum/quadsort) if the array is less than 25% random.

Partitioning
------------
Partitioning is performed in a top-down manner similar to quicksort. Fluxsort obtains the pseudomedian of 9 for partitions smaller than 1024 elements, and the pseudomedian of 25 otherwise. The median element obtained will be referred to as the pivot. Partitions that grow smaller than 24 elements are sorted with quadsort.

After obtaining a pivot the array is parsed from start to end. Elements smaller than the pivot are copied in-place to the start of the array, elements greater than the pivot are copied to swap memory. The partitioning routine is called recursively on the two partitions in main and swap memory.

Recursively partitioning through both swap and main memory is accomplished through the ptx pointer, which despite being simple in implementation is likely a novel technique since the logic behind it is a bit of a brain-twister.

Worst case handling
-------------------
To avoid run-away recursion fluxsort switches to quadsort for both partitions if one partition is less than 1/16th the size of the other partition. On a distribution of random unique values the observed chance of a false positive is 1 in 1,336 for the pseudomedian of 9 and approximately 1 in 4 million for the pseudomedian of 25.

Combined with the analyzer fluxsort starts out with this makes the existence of killer patterns unlikely, other than a 2x performance slowdown by prematurely triggering the use of quadsort on random data.

Branchless optimizations
------------------------
Fluxsort uses a branchless comparison optimization similar to the method described in "BlockQuicksort: How Branch Mispredictions don't affect Quicksort" by Stefan Edelkamp and Armin Weiss.

Median selection uses a novel branchless comparison technique that selects the pseudomedian of 9 using between 8 and 12 (10.66 average) comparisons, and the pseudomedian of 25 using between 24 and 60 (48 average) comparisons.

These optimizations do not work as well when the comparisons themselves are branched and the largest performance increase is on 32 and 64 bit integers.

Generic data optimizations
--------------------------
Fluxsort uses a method popularized by [pdqsort](https://github.com/orlp/pdqsort) to improve generic data handling. If the same pivot is chosen twice in a row it performs a reverse partition, filtering out all elements equal to the pivot, next it carries on as usual. This typically only occurs when sorting tables with many repeating values, like gender, education level, birthyear, zipcode, etc.

Large array optimizations
-------------------------
For partitions larger than 65536 elements fluxsort obtains the median of 128 or 256. It does so by copying 128 or 256 random elements to swap memory, sorting them with fluxsort, and taking the center element. Using pseudomedian instead of median selection on large arrays is slower, likely due to cache pollution.

Memory
------
Fluxsort allocates n elements of swap memory, which is shared with quadsort. Recursion requires log n stack memory.

If memory allocation fails fluxsort defaults to quadsort, which requires n / 4 elements of swap memory. If allocation fails again quadsort will sort in-place through rotations.

Data Types
----------
The C implementation of fluxsort supports long doubles and 8, 16, 32, and 64 bit data types. By using pointers it's possible to sort any other data type, like strings.

Interface
---------
Fluxsort uses the same interface as qsort, which is described in [man qsort](https://man7.org/linux/man-pages/man3/qsort.3p.html).

Big O
-----
```cobol
                 ┌───────────────────────┐┌───────────────────────┐
                 │comparisons            ││swap memory            │
┌───────────────┐├───────┬───────┬───────┤├───────┬───────┬───────┤┌──────┐┌─────────┐┌─────────┐
│name           ││min    │avg    │max    ││min    │avg    │max    ││stable││partition││adaptive │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│fluxsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││yes      ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quadsort       ││n      │n log n│n log n││1      │n      │n      ││yes   ││no       ││yes      │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│quicksort      ││n      │n log n│n²     ││1      │1      │1      ││no    ││yes      ││no       │
├───────────────┤├───────┼───────┼───────┤├───────┼───────┼───────┤├──────┤├─────────┤├─────────┤
│introsort      ││n log n│n log n│n log n││1      │1      │1      ││no    ││yes      ││no       │
└───────────────┘└───────┴───────┴───────┘└───────┴───────┴───────┘└──────┘└─────────┘└─────────┘
```

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
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and std::stable_sort are inlined.

![fluxsort vs stdstablesort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_stdstablesort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   64 | 0.006040 | 0.006092 |         1 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.002032 | 0.002051 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   32 | 0.006076 | 0.006109 |         1 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001888 | 0.001903 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003057 | 0.003079 |         1 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000485 | 0.000494 |         1 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.004953 | 0.004983 |         1 |     100 |    random % 1000 |
|  fluxsort |   100000 |   32 | 0.000978 | 0.000992 |         1 |     100 |    random % 1000 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003852 | 0.003917 |         1 |     100 |   square % 10000 |
|  fluxsort |   100000 |   32 | 0.000978 | 0.001054 |         1 |     100 |   square % 10000 |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000783 | 0.000843 |         1 |     100 |        ascending |
|  fluxsort |   100000 |   32 | 0.000047 | 0.000048 |         1 |     100 |        ascending |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001465 | 0.001504 |         1 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000859 | 0.000867 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000872 | 0.000891 |         1 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000214 | 0.000216 |         1 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000909 | 0.000921 |         1 |     100 |       descending |
|  fluxsort |   100000 |   32 | 0.000057 | 0.000057 |         1 |     100 |       descending |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001098 | 0.001113 |         1 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000407 | 0.000420 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.002134 | 0.002176 |         1 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.001384 | 0.001392 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003594 | 0.003634 |         1 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001822 | 0.001840 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001106 | 0.001128 |         1 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.001013 | 0.001019 |         1 |     100 |  ascending tiles |

</details>

The following benchmark was on WSL gcc version 7.4.0 (Ubuntu 7.4.0-1ubuntu1~18.04.1).
The source code was compiled using gcc -O3 bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and qsort are not inlined. The stdlib qsort() in the benchmark is a mergesort variant. 

![fluxsort vs qsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_qsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.016662 | 0.016878 |   1536548 |     100 |    random string |
|  fluxsort |   100000 |   64 | 0.010894 | 0.011155 |   1987272 |     100 |    random string |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |  128 | 0.018280 | 0.018835 |   1536363 |     100 |     random order |
|  fluxsort |   100000 |  128 | 0.011630 | 0.011728 |   1990256 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.009275 | 0.009348 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.004570 | 0.004614 |   1977809 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   32 | 0.008465 | 0.008547 |   1536634 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.004029 | 0.004081 |   1991219 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.006409 | 0.006470 |   1532465 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.001423 | 0.001458 |    968722 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002020 | 0.002132 |    815024 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000161 | 0.000162 |     99999 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002823 | 0.003056 |    915020 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.001461 | 0.001478 |    558848 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002341 | 0.002392 |    884462 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000657 | 0.000669 |    404041 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002466 | 0.002500 |    853904 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000154 | 0.000154 |     99999 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003238 | 0.003418 |    953892 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.001449 | 0.001463 |    570729 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003735 | 0.003837 |   1012028 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.002229 | 0.002251 |    681125 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005446 | 0.005493 |   1200835 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.003586 | 0.003628 |   1886276 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003872 | 0.003919 |   1209200 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.003334 | 0.003350 |   1361302 |     100 |  ascending tiles |

</details>

The following benchmark was on WSL gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04) using the [wolfsort](https://github.com/scandum/wolfsort) benchmark.
The source code was compiled using g++ -O3 -w -fpermissive bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and pdqsort are inlined.

![fluxsort vs pdqsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_pdqsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   64 | 0.002689 | 0.002718 |         1 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.002034 | 0.002061 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|   pdqsort |   100000 |   32 | 0.002680 | 0.002694 |         1 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.001890 | 0.001906 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000477 | 0.000483 |         1 |     100 |     random % 100 |
|  fluxsort |   100000 |   32 | 0.000485 | 0.000496 |         1 |     100 |     random % 100 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.001218 | 0.001233 |         1 |     100 |    random % 1000 |
|  fluxsort |   100000 |   32 | 0.000979 | 0.000994 |         1 |     100 |    random % 1000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.001258 | 0.001268 |         1 |     100 |   square % 10000 |
|  fluxsort |   100000 |   32 | 0.000978 | 0.001041 |         1 |     100 |   square % 10000 |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000084 | 0.000084 |         1 |     100 |        ascending |
|  fluxsort |   100000 |   32 | 0.000046 | 0.000048 |         1 |     100 |        ascending |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.003368 | 0.003389 |         1 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000855 | 0.000871 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002829 | 0.002851 |         1 |     100 |       pipe organ |
|  fluxsort |   100000 |   32 | 0.000214 | 0.000220 |         1 |     100 |       pipe organ |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.000188 | 0.000191 |         1 |     100 |       descending |
|  fluxsort |   100000 |   32 | 0.000058 | 0.000059 |         1 |     100 |       descending |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002322 | 0.002343 |         1 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000407 | 0.000422 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002580 | 0.002602 |         1 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.001381 | 0.001394 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002644 | 0.002662 |         1 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.001821 | 0.001844 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|   pdqsort |   100000 |   32 | 0.002310 | 0.002328 |         1 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.001013 | 0.001020 |         1 |     100 |  ascending tiles |

</details>
