[![Actions Status](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/workflows/MacOS/badge.svg)](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/actions)
[![Actions Status](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/workflows/Windows/badge.svg)](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/actions)
[![Actions Status](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/workflows/Ubuntu/badge.svg)](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/actions)
[![Actions Status](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/workflows/Style/badge.svg)](https://github.com/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/actions)
[![codecov](https://codecov.io/gh/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons/branch/master/graph/badge.svg)](https://codecov.io/gh/andersfylling/sorting-nine-inputs-requires-twenty-five-comparisons)

# About
This is a **third-party** implementation of the paper ["*Sorting nine inputs requires twenty-five comparisons*"](https://www.sciencedirect.com/science/article/pii/S0022000015001397) [1] in c++. It exists to make a reliable reference point for future projects that needs to measure the runtime reduction of their ideas/findings, with regards to [the minimum comparator network size problem](https://en.wikipedia.org/wiki/Sorting_network) [3].

This project is considered feature complete as the core methods are implemented in a naive/simple manner of their prolog version. However, code design, naming, or anything that improves readability are changes of interest. 

## Features

- Multi-threading
- Scalable memory usage
- Minimal heap allocations
- Test suite
- Components as a static library

TODO: benchmark

## Code flow
The goal is to find a network with size K that has no smaller network able to sort a sequence of N elements. In order to do so a weak proof algorithm must be implemented that proves no smaller network exist by exploring all configurations. This is also known as brute forcing. As such the program starts with a network of zero comparators and derives all possible configurations using a [breadth first search](https://en.wikipedia.org/wiki/Breadth-first_search) [4] until a sorting network is discovered. Because every network configuration of size K is explored step wise, we know that there is no network of size K-1 that can sort a sequence of N elements - which ultimately becomes the proof that the discovered sorting network is in fact the smallest size.  

However, there is no need to explore every single network as the comparators are independent functions - instead the output sets can be generated using the [zero-one principle](http://www.euroinformatica.ro/documentation/programming/!!!Algorithms_CORMEN!!!/DDU0170.html) [5] and redundant networks can be pruned using subsumption. This is further improved upon by permuting the output set to take advantage of breaking symmetries within a step or layer (where the sets compared comes from networks with equal size).

The generate and prune approach, is what it sounds; it generates all the networks, then identifies redundant networks through symmetry and prune redundant ones. This is the basic concept of the search space. The paper [1] explores preconditions for identifying whether two output sets may subsume one another - that are much quicker than the complete subsumption test. These are referred to as ST1, ST2, ST3 in the code.

This project differs from the prolog code, by working with segments of networks and their output sets. It is not explicitly clear to me how prolog does this internally - regardless, by working on segments instead of singular networks it becomes easier to visualize the multi-threading aspects of the code. Once work is done on a segment, the result or updated segment is written it's own file. Networks and output sets do not share the same file, but merely a segment ID to reduce overall IO. From my understanding the prolog version, for every layer, writes the generated networks and output sets to a single file and then the pruned version to a new file once that layer is fully explored. 

### Multi-threading

Each layer can be split up into 3 parts; generating, pruning within segments (files) and pruning across segments (files). The generator phase is single threaded as it only wastes a few minutes on N9, while the multi-threaded pruning phases can take several hours to complete. 

Pruning within segments (files) tells each thread to work on a single segment. Since each segment is isolated, there is no need for locking and the implementation quickly becomes IO bound as the number of sets/networks reduces per segment on N9. But as N increases the complexity of pruning may go beyond the IO penalties.

![](.github/multithreading-within-segments.gif)

Pruning across segments (files) have the same issue with IO, but must also share memory across threads. After talking with an author from the paper [1] the approach was to mark one segment as read only, and the remaining segments as writeable. Where a writeable segment can only have ownership by one thread. Then every thread can share the read-only segment and see if any of the output sets subsumes any output set in the write-able segment the thread has ownership off. Once every write-able segment has been compared to the read-only segment, the process selects a different segment to be marked as read-only and the rest as write-able. Thanks to this, every output set is correctly compared across segments without the need for synchronization between jobs.

![](.github/multithreading-across-segments.gif)


## Contributing

### Build and run the standalone target

Use the following command to build and run the executable target.

```bash
cmake -Happ -Bbuild/app
cmake --build build/app
./build/app/Sortnet --help
```

### Build and run test suite

Use the following commands from the project's root directory to run the test suite.

```bash
cmake -Htest -Bbuild/test
cmake --build build/test
CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test

# or simply call the executable: 
./build/test/GreeterTests
```

To collect code coverage information, run CMake with the `-DENABLE_TEST_COVERAGE=1` option.

### Run clang-format

Use the following commands from the project's root directory to run clang-format (must be installed on the host system).

```bash
cmake -Htest -Bbuild/test

# view changes
cmake --build build/test --target format

# apply changes
cmake --build build/test --target fix-format
```

See [Format.cmake](https://github.com/TheLartians/Format.cmake) for more options.

## References

 - [1] https://www.sciencedirect.com/science/article/pii/S0022000015001397
 - [2] https://www.researchgate.net/publication/318729549_An_Improved_Subsumption_Testing_Algorithm_for_the_Optimal-Size_Sorting_Network_Problem
 - [3] https://en.wikipedia.org/wiki/Sorting_network
 - [4] https://en.wikipedia.org/wiki/Breadth-first_search
 - [5] http://www.euroinformatica.ro/documentation/programming/!!!Algorithms_CORMEN!!!/DDU0170.html
