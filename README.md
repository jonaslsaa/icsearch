# Interaction Combinators Research: Factorization via Universal Search

This repository demonstrates how **Interaction Combinators (IC)** can be used to build a universal search system. As a **working example**, it attempts to factor an integer by enumerating and evaluating IC networks under a configurable resource limit.

## Table of Contents

- [Interaction Combinators Research: Factorization via Universal Search](#interaction-combinators-research-factorization-via-universal-search)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Repository Structure](#repository-structure)
  - [Build and Run](#build-and-run)
    - [Prerequisites](#prerequisites)
    - [Building](#building)
    - [Running](#running)
    - [Testing](#testing)
  - [Key Concepts](#key-concepts)
    - [IC Runtime](#ic-runtime)
    - [Universal Search](#universal-search)
    - [Factoring Example](#factoring-example)
  - [How It Works](#how-it-works)
  - [Performance Optimizations](#performance-optimizations)
  - [Advanced Topics](#advanced-topics)

---

## Overview

**Interaction Combinators** are a model of computation defined by local graph rewrites on three combinators (δ, γ, ε). This project:

1. Implements a **runtime** that creates, connects, and rewrites IC nodes (under a *gas limit* to avoid infinite loops).
2. Implements a **search** that systematically enumerates possible IC networks up to a specified size, executes them, and checks if they solve a target problem.
3. Demonstrates factoring an integer using this "universal search" approach.

**Note**: This is a research-oriented codebase, not a production library. The factoring example is mostly illustrative, showing how a net that rewires correctly can "discover" factors.

---

## Repository Structure

```
.
├── src/
│   ├── ic_runtime.c   # IC runtime logic (node mgmt, rewrite rules, etc.)
│   ├── ic_runtime.h
│   ├── ic_search.c    # Enumeration/search logic
│   ├── ic_search.h
│   ├── ic_enum.c      # Optimization of enumeration process 
│   ├── main.c         # CLI tool that factors an integer using the search
│   └── main_test.c    # Test suite
├── Makefile           # Build script
├── CLAUDE.md          # Additional development guidelines
├── SPEC.md            # Detailed specification & design document
└── .gitignore
```

- **`ic_runtime.[ch]`**: Core IC data structures and rewrite mechanics with redex queue optimization.
- **`ic_search.[ch]`**: Enumerates and evaluates IC nets, checking if they yield a factorization.
- **`ic_enum.c`**: Implements optimized enumeration with fast pattern-based connections.
- **`main.c`**: Command-line interface for factoring a given integer.
- **`main_test.c`**: Self-contained test program covering various features (rewrites, gas limits, etc.).
- **`Makefile`**: Provides targets for building and testing.
- **`CLAUDE.md`**: Development guidelines, suggested linting, memory checks, and best practices.
- **`SPEC.md`**: Comprehensive specification covering the system architecture, usage, and extension points.

---

## Build and Run

### Prerequisites

- A C11-compatible compiler (e.g. `gcc`, `clang`).
- Basic command-line tools (e.g. `make`).

### Building

```bash
make
```

This produces two main executables:
1. **`main`** – The primary CLI tool for factoring.
2. **`test`** – The test suite executable.

### Running

```bash
./main <number_to_factor> [max_nodes] [gas_limit]
```

- **`<number_to_factor>`**: The integer N you want to factor (must be > 1).
- **`[max_nodes]`**: Optional upper bound on the size of enumerated IC nets (default: 100).
- **`[gas_limit]`**: Optional limit on how many rewrite steps to allow before halting (default: 100000).

Example:

```bash
./main 30 50 10000
```

This attempts to factor 30, enumerating possible IC networks up to 50 nodes, and limiting each network to 10,000 rewrite steps.

### Testing

```bash
make test
./test
```

The test suite runs standalone and checks:
- Node creation limits
- Port connections
- Various rewrite scenarios (δ–δ, γ–γ, δ–γ, ε interactions)
- Gas limit behavior
- Factorization checks
- Enumeration correctness

---

## Key Concepts

### IC Runtime

- **Nodes**: Each node has type (δ, γ, or ε) and three ports (principal and two auxiliaries).
- **Connections**: Ports connect to other ports (bidirectional).  
- **Rewriting**: If two nodes' *principal* ports are connected, they form an *active pair*. A local rewrite rule is applied based on the pair's types, potentially rewiring ports or creating new nodes.
- **Redex Queue**: A performance optimization that tracks active pairs in a queue instead of rescanning the entire net after each rewrite.
- **Gas Limit**: Each rewrite step consumes "gas." If the net rewrites too many steps, it stops early.

### Universal Search

- **Enumeration**: The system systematically generates candidate IC networks up to `max_nodes`. Each network is built from a numeric "index" that encodes node types and connections.
- **Execution**: For each enumerated net, the IC runtime runs it.  
- **Check**: If the net's final state encodes a factorization for a given integer N, the search returns a solution.

### Factoring Example

- We store the integer `N` in the net structure (`net->input_number`).  
- After reduction, if the net sets `net->factor_a` and `net->factor_b` such that `factor_a * factor_b == N`, we declare success.

---

## How It Works

1. **CLI** (`main.c`) parses N, `max_nodes`, and `gas_limit`.  
2. **ic_enum_state_t** initialized to track which networks have been generated.  
3. Loop over potential indices, **build** each network (`ic_enum_build_net`) in memory, assigning node types and connections.  
4. Store `N` in `net->input_number`.  
5. **Reduce** the network (`ic_net_reduce`) until no active pairs remain or gas is exhausted.  
6. **Check** if `net->factor_found` and `net->factor_a * net->factor_b == N`.  
7. If found, print the factors and optionally export a DOT file (`solution.dot`) for visualization.

---

## Performance Optimizations

- **Redex Queue**: Instead of rescanning the entire net for active pairs after each rewrite operation, we maintain a queue of redexes (active pairs) that need to be processed. New potential redexes are added to the queue when ports are connected.

- **Fast Reset**: When building nets from enumeration indices, we use efficient bit-pattern encoding and decoding to quickly construct the graph structure.

- **Optimized Connection Process**: The connection logic in ic_enum.c efficiently handles the creation of complex port connections with minimal overhead.

- **Selective Scanning**: When the redex queue is empty, we only rescan the net once rather than after every operation, significantly reducing the overhead for highly connected nets.

---

## Advanced Topics

- **Rewrite Rules**: See [SPEC.md](SPEC.md) or `ic_runtime.c` for details of δ–δ, γ–γ, δ–γ, and ε–anything interactions.
- **Performance**: Large `max_nodes` or a high `gas_limit` can slow the search dramatically. Consider distributing the index range across multiple machines or threads for bigger explorations.
- **Visualization**: You can run `ic_net_export_dot` to produce Graphviz DOT files of the final or intermediate nets:
  ```bash
  dot -Tpng solution.dot -o solution.png
  ```
- **Extensibility**: Although factoring is the demonstration, you can adapt or extend the search logic to solve other tasks by customizing how the net is checked after reduction.