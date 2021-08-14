Intro
-----
This document describes a partitioning stable adaptive comparison-based sort named fluxsort. Benchmarks and a visualization are available at the bottom.

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

Memory
------
Fluxsort allocates n elements of swap memory, which is shared with quadsort. Recursion requires log n stack memory.

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
|stablesort |   100000 |   64 | 0.006123 | 0.006153 |         1 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.002477 | 0.002488 |         1 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |     Loops | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|stablesort |   100000 |   32 | 0.006008 | 0.006033 |         1 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.002329 | 0.002345 |         1 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000679 | 0.000683 |         1 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000037 | 0.000037 |         1 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001375 | 0.001402 |         1 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.000842 | 0.000854 |         1 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003827 | 0.003853 |         1 |     100 |    generic order |
|  fluxsort |   100000 |   32 | 0.001129 | 0.001140 |         1 |     100 |    generic order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000901 | 0.000912 |         1 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000048 | 0.000048 |         1 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.001021 | 0.001035 |         1 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.000351 | 0.000365 |         1 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.002034 | 0.002060 |         1 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.001485 | 0.001492 |         1 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.003520 | 0.003542 |         1 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.002077 | 0.002088 |         1 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|stablesort |   100000 |   32 | 0.000922 | 0.000956 |         1 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000674 | 0.000692 |         1 |     100 |  ascending tiles |

</details>

The following benchmark was on WSL gcc version 7.4.0 (Ubuntu 7.4.0-1ubuntu1~18.04.1).
The source code was compiled using gcc -O3 bench.c. The bar graph shows the best run out of 100 on 100,000 32 bit integers. Comparisons for fluxsort and qsort are not inlined. The stdlib qsort() in the benchmark is a mergesort variant. 

![fluxsort vs qsort](https://github.com/scandum/fluxsort/blob/main/images/fluxsort_vs_qsort.png)

<details><summary>data table</summary>

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   64 | 0.009427 | 0.009512 |   1536491 |     100 |     random order |
|  fluxsort |   100000 |   64 | 0.004853 | 0.004862 |   2001035 |     100 |     random order |

|      Name |    Items | Type |     Best |  Average |  Compares | Samples |     Distribution |
| --------- | -------- | ---- | -------- | -------- | --------- | ------- | ---------------- |
|     qsort |   100000 |   32 | 0.008472 | 0.008609 |   1536634 |     100 |     random order |
|  fluxsort |   100000 |   32 | 0.004117 | 0.004136 |   1990342 |     100 |     random order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002017 | 0.002194 |    815024 |     100 |  ascending order |
|  fluxsort |   100000 |   32 | 0.000140 | 0.000140 |     99999 |     100 |  ascending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002817 | 0.002855 |    915019 |     100 |    ascending saw |
|  fluxsort |   100000 |   32 | 0.001514 | 0.001525 |    558847 |     100 |    ascending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.006382 | 0.006435 |   1532339 |     100 |    generic order |
|  fluxsort |   100000 |   32 | 0.002333 | 0.002349 |   1269601 |     100 |    generic order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002450 | 0.002479 |    853904 |     100 | descending order |
|  fluxsort |   100000 |   32 | 0.000150 | 0.000150 |     99999 |     100 | descending order |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.002826 | 0.002916 |   1063907 |     100 |   descending saw |
|  fluxsort |   100000 |   32 | 0.001171 | 0.001200 |    697343 |     100 |   descending saw |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003705 | 0.003751 |   1012028 |     100 |      random tail |
|  fluxsort |   100000 |   32 | 0.002251 | 0.002261 |    681125 |     100 |      random tail |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.005447 | 0.005497 |   1200835 |     100 |      random half |
|  fluxsort |   100000 |   32 | 0.003728 | 0.003747 |   1889402 |     100 |      random half |
|           |          |      |          |          |           |         |                  |
|     qsort |   100000 |   32 | 0.003873 | 0.004301 |   1209200 |     100 |  ascending tiles |
|  fluxsort |   100000 |   32 | 0.000999 | 0.001005 |    400063 |     100 |  ascending tiles |

</details>
