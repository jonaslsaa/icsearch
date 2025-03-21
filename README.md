# icsearch: Interaction Combinators Universal Search

A universal search system built on Interaction Combinators (IC) with applications to integer factorization.

## Overview

This project implements:

1. **IC Runtime Module**
   - Compact index-based graph representation for IC nodes
   - Interaction (rewrite) rules for δ, γ, and ε combinators
   - Bounded execution with gas limit

2. **Universal Search Module**
   - Enumerates all possible IC programs within a size limit
   - Executes each enumerated graph using the IC runtime
   - Demonstration: finding a factorization of a given integer

## Usage

```
./main <number_to_factor> [max_nodes] [gas_limit]
```

Example:
```
./main 15 100 10000
```

The program will search for an IC net that factors the given number (15) using up to 100 nodes and 10000 reduction steps.

## Build Instructions

```bash
# Build the main executable
make

# Build the test suite
make test

# Run all tests
./test

# Check memory leaks
valgrind --leak-check=full ./main <input>
```

## Design

The system consists of three main components:

```
      +--------------------+
      |   main.c (CLI)     |
      +--------------------+
               |
               v
      +--------------------+
      | Universal Search   |  <-- Enumerates IC programs
      |  (ic_search.[ch])  |      under memory & gas limits
      +--------------------+
               |
               v
      +--------------------+
      | IC Runtime         |  <-- Creates and runs net
      | (ic_runtime.[ch])  |      with rewriting logic
      +--------------------+
```

When the search finds a valid factorization, it generates a visual representation of the solution:
```
You can visualize it with: dot -Tpng solution.dot -o solution.png
```

## Development Guidelines

- C11 standard with no external dependencies
- Fixed-size arrays with explicit capacity limits
- snake_case naming convention
- Clear separation between IC runtime and search logic
- Return codes (-1 for errors, 0 for success)
- Comprehensive unit tests in main_test.c

## License

Copyright (c) 2025