[![Actions Status](https://github.com/TheLartians/ModernCppStarter/workflows/MacOS/badge.svg)](https://github.com/TheLartians/ModernCppStarter/actions)
[![Actions Status](https://github.com/TheLartians/ModernCppStarter/workflows/Windows/badge.svg)](https://github.com/TheLartians/ModernCppStarter/actions)
[![Actions Status](https://github.com/TheLartians/ModernCppStarter/workflows/Ubuntu/badge.svg)](https://github.com/TheLartians/ModernCppStarter/actions)
[![Actions Status](https://github.com/TheLartians/ModernCppStarter/workflows/Style/badge.svg)](https://github.com/TheLartians/ModernCppStarter/actions)
[![codecov](https://codecov.io/gh/TheLartians/ModernCppStarter/branch/master/graph/badge.svg)](https://codecov.io/gh/TheLartians/ModernCppStarter)

# About
This is a _*third-party*_ c++ implementation of the paper "*Sorting nine inputs requires twenty-five comparisons*"[1]. It exists to make a reliable reference point for other researchers that wants to compare the performance of their ideas/findings.

The core logic will not receive further changes as it is considered "complete" - in the sense that the implementation is correct. However, code design, naming, or anything that improves on readability are acceptable changes.
 

## Features

- Multi-threading
- Scalable memory usage (segments of networks and sets are written/read from files)
- Minimal heap allocations
- Test suite
- Somewhat modular code

## Core logic
This section details the significant findings that allows reduces the problem space.

### Lemma 4

> Let Ca and Cb be n-channel comparator networks. If there exists 1 ≤ k ≤ n such that the number of sequences with k 1s in outputs(Ca) is greater than that in outputs(Cb), then Ca does not subsumes Cb.

This lemma states that after we partition a set based on the number of set bits in the entries, we can check if two networks subsumes by checking the number of entries in each partition.

##### Example
Say we have the set `A={001, 010, 011}` and the set `B={010, 011, 101}`. I denote part(S, k) for a partition of a set S where the entries have k bits. eg. `part({010, 110}, 1) => {010}`. 
By taking the partition k(1) for set A and B, we see that part(A, k) has more elements than part(B, k), where k is 1.
```
  part(A, 1) => {001, 010} 
  part(B, 1) => {010}
```

As such, A can not subsumes B. And there is no reason to do further more expensive tests.

The effect of this lemma is that instead of testing if every entry in A exists in B, which has a potential worst case of O(N^2 * N^2), we can use lemma 4 as a precondition and reduce the worst case to O(N-1). Allowing for fail fast, or quickly skipping a significant number of checks. In the paper they stated than more than 70% of the subsumption tests where eliminated thanks to lemma 4.

### Lemma 5

> Let Ca and Cb be n-channel comparator networks. If for some x∈{0, 1} and 1 ≤ k ≤ n, |w(Ca, x, k)| > |w(Cb, x, k)| then Ca does not subsume Cb.

This lemma states that the sizes of a condensed representation of all the activated bits and the non-activated bits, the ones and the zeros, can be used as another precondition for subsumption. This again, works on a per partition basis, which makes up for some of the inaccuracy of using a condenced view.

The focus is on the number of positions a activated bit can move around. When a permutation is applied, the number of activated bits in the once and zero representation for each partition do not change and therefore we can check for subsumption by number of ones and zeros.

##### Example
In this example we focus on a specific partition, instead of writing an entire set. Remember that a partition must be able to subsume another where the number of activated bits are the same.

```
part(A, 2) = {00011, 10010, 01010}
part(B, 2) = {00011, 01001, 01010}
``` 
Lemma 4 would not detect this situation as it notices the size of size(part(A, 2)) subsumes size(part(B, 2)). 

But if we created the ones and the zeros for each of the sets respectively, we notice something stands out.
```
ones(part(A, 2)) = {0,2,4,5}
ones(part(B, 2)) = {2,4,5}
```

We see that the ones for part(A, 2) has more activated bits than part(B, 2). By lemma 5, this states that A can not subsume B.

#### Example - calculate ones and zeros
Given a binary sequence s1 = 0101101, we can calculate the ones(s1) = {1,3,4,6}. In the papers we assign the first position 0, starting from left to right. And we can write s1 as a ordered set instead: s1 = (0, 1, 0, 1, 1, 0, 1).
Looping over this ordered set, we record when the value is 1:

```
def ones(sequence):
    ones = []
    for bit, pos in sequence:
        if bit == 1:
            ones.append(pos)
    return ones
```

To calculate the zeros of a sequence, we simply take the inverse of the sequence: ~s1 = 1010010. zeros(s1) = {0, 2, 5}.

```
def zeros(sequence):
    inverted = ~sequence
    return ones(inverted)
```

We can now use this logic to create a concensed representation of a set. Below we define the psuedo code to make this work.
```
def ones(set):
    positions = []
    for sequence in set:
        for pos in ones(sequence):
            if positions.contains(pos):
                continue
            positions.append(pos)
    return positions

def zeros(set):
    for i in range(set):
        set[i] = ~set[i]
    return ones(set)
```

### Lemma 6

> Let Ca and Cb be n-channel comparator networks and π be a permutation. If π(outputs(Ca)) ⊆ outputs(Cb), then π(w(Ca, x, k)) ⊆ w(Cb, x, k) for all x∈{0, 1}, 1 ≤ k ≤ n.

The main idea of this is to show that we can use the ones and zeros from each partition to figure out or generate plausible permutations. Due to the condenced nature of the ones and zeros, this yields some inaccuracy and will generate permutations that will not cause the two sets to subsume. However, if they subsume a permutation, this will find it.
You can think about the valid permutations for this statement to exist as a subset within the generated permutations using the ones and the zeros from each partition.

This is probably the most significant statement that allows scaling. While exploring all permutations of N (N!) can take a few seconds for N7, it took me 20 hours for N8 on a single core. Once I moved to generating the permutations instead the runtime of N8 reduced to minutes.

There have previously been misinterpretations of lemma 6[1, page 9] where at first glance it seems that all possible permutations for a N-sized sequence are explored[2], as in applying N! permutations and test for subsumption. This is simply not the case as the prolog implementation uses backtracking to generate possible permutations. I tried exploring all permutations for N8 in this way, and it took the C++ implementation around 20 hours on a single core, while the prolog version takes 3 hours (one of the authors[1] let me try their code on my personal computer).

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
