Below is a **comprehensive specification** that you can hand off to a team of senior C engineers. It details a robust, scalable design for **(1) an Interaction Combinators (IC) runtime** and **(2) a Universal Search engine** that enumerates IC programs. The spec is written around C11 best practices, focuses on **bounded memory usage**, and includes testability guidelines. The factoring task is the first practical use case, but the design remains general-purpose.

---

# 1. Overview

This project delivers a **universal search** system built on **Interaction Combinators (IC)**. It has two core modules:

1. **IC Runtime Module**  
   - A compact index-based graph representation for IC nodes.  
   - Implements creation, connection, reduction (rewriting), and step-limiting (gas model).

2. **Universal Search Module**  
   - Enumerates all possible IC programs within a given size limit.  
   - Passes each enumerated graph to the IC Runtime for execution under specified resource bounds.  
   - Initially demonstrates factoring an integer, but extensible to other tasks.

A **main** program ties these modules together as a CLI tool that, for demonstration, factors a given number.

---

# 2. Project Goals & Requirements

1. **C11 Self-Contained**: No external libraries. Only standard headers and standard library.
2. **Modular Architecture**:  
   - Clear separation of the **IC Runtime** and the **Search** logic.  
   - A small, dedicated CLI in `main.c`.
3. **Bounded Memory**:  
   - Fixed-size memory pools or arrays for storing IC nodes.  
   - Limit on the number of nodes per IC program.  
   - A configurable “gas” limit (max rewrite steps).
4. **Testability**:  
   - A single file `main_test.c` for correctness tests.  
   - Each module has well-defined functions that are testable in isolation.
5. **Initial Use Case**: Factor a user-provided integer. The net is “correct” if it eventually produces a valid factorization. 

---

# 3. High-Level Architecture

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

1. **main.c**  
   - Parses CLI arguments.  
   - Invokes the Universal Search with a target number to factor.
   - Waits for the search to find a net that yields a valid factorization.

2. **ic_search.[ch]**  
   - Enumerates IC programs via a deterministic index-based scheme.  
   - For each enumerated program (graph), calls into IC Runtime to execute it.  
   - Checks if the result meets a success criterion (e.g., factorization correctness).

3. **ic_runtime.[ch]**  
   - Manages the data structures for IC nodes, ports, and connections.  
   - Applies local rewrite rules (δ, γ, ε).  
   - Enforces gas limits and memory bounds.  
   - Provides functions to define custom input/outputs for special tasks like factoring.

4. **main_test.c**  
   - Comprehensive tests for correctness: net creation, rewriting, memory boundary tests, enumeration tests, factoring tests, etc.

---

# 4. Detailed Module Specifications

## 4.1 Interaction Combinators Module: `ic_runtime.[ch]`

### 4.1.1 Data Structures

1. **ic_node_type_t**  
   ```c
   typedef enum {
       IC_NODE_DELTA,   // δ
       IC_NODE_GAMMA,   // γ
       IC_NODE_EPSILON  // ε
   } ic_node_type_t;
   ```

2. **ic_port_t**  
   - Each node has 3 ports: *principal*, *auxiliary1*, *auxiliary2* (or whichever labeling scheme you prefer).  
   - We store a connection index that points to another (node, port).

   ```c
   typedef struct {
       int connected_node;  // Index of the connected node
       int connected_port;  // Which port on that node
   } ic_port_t;
   ```

3. **ic_node_t**  
   ```c
   typedef struct {
       ic_node_type_t type;
       ic_port_t ports[3]; // principal = ports[0], aux1 = ports[1], aux2 = ports[2]
   } ic_node_t;
   ```

4. **ic_net_t**  
   ```c
   typedef struct {
       ic_node_t *nodes;     // Array of nodes in this net
       size_t max_nodes;     // Capacity (bound on total nodes)
       size_t used_nodes;    // How many nodes currently allocated
       size_t gas_limit;     // Maximum rewrite steps
   } ic_net_t;
   ```

   - `nodes` is a **fixed-size** array for bounding memory usage.  
   - `used_nodes` tracks how many are currently in use.

### 4.1.2 Public API

1. **Net Creation/Destruction**  
   ```c
   ic_net_t *ic_net_create(size_t max_nodes, size_t gas_limit);
   void ic_net_free(ic_net_t *net);
   ```
   - Allocates a new net structure with a pre-allocated array of `max_nodes`.  
   - `gas_limit` is the maximum rewrite steps (partial gas model).

2. **Node Allocation**  
   ```c
   int ic_net_new_node(ic_net_t *net, ic_node_type_t type);
   ```
   - Returns the index of the newly created node, or `-1` on error (no space left).

3. **Connection Management**  
   ```c
   void ic_net_connect(ic_net_t *net, int node_a, int port_a, int node_b, int port_b);
   ```
   - Connects the specified port of `node_a` to the specified port of `node_b`.
   - Automatically updates both sides (bidirectional).

4. **Reduction (Evaluation)**  
   ```c
   int ic_net_reduce(ic_net_t *net);
   ```
   - Runs the rewriting until:
     - No active pairs remain, OR
     - `gas_limit` is reached.
   - Returns 0 if fully reduced, or 1 if it halted due to gas exhaustion.

5. **Utility**  
   ```c
   // For debugging or test display
   void ic_net_print(const ic_net_t *net);

   // (Optional) Retrieve node count, check connectivity, etc.
   size_t ic_net_get_used_nodes(const ic_net_t *net);
   ```

### 4.1.3 Rewrite Logic

- We assume standard **Interaction Combinators** with three agent types `(δ, γ, ε)` and the known local rules for rewiring.
- Each step in `ic_net_reduce`:
  1. **Scan** the net for an **active pair** (two nodes connected via principal ports).
  2. **Rewrite** them according to the combinator rules:
     - Possibly remove nodes, create new ones, or rewire existing connections.
  3. **Decrement gas** each time a rewrite rule is applied.
  4. Stop if no active pair or if `gas == 0`.

### 4.1.4 Error Handling

- **Allocation Errors**: If `ic_net_new_node` runs out of `max_nodes`, return `-1`.
- **Connection Errors**: If node indices or port indices are out of range, do nothing or log an error.
- **Rewrite Errors**: Typically not expected in a well-formed net, but if encountered, can set an internal error flag.

---

## 4.2 Universal Search Module: `ic_search.[ch]`

### 4.2.1 Overview

Responsible for enumerating possible IC program graphs in a **bounded space**:
1. A maximum number of nodes, `MAX_NODES`.
2. A systematic enumeration approach for each possible graph structure within these constraints.
3. For each enumerated net:
   - Construct the net in memory via the `ic_runtime` API.
   - Reduce it (gas-limited).
   - Check if the result solves the factoring problem (or other tasks).

### 4.2.2 Data Structures

- **ic_enum_state_t**  
  ```c
  typedef struct {
      size_t max_nodes;
      size_t current_index;
      // Possibly other fields to handle enumerations in progress
  } ic_enum_state_t;
  ```

### 4.2.3 Public API

1. **Initialization**  
   ```c
   void ic_enum_init(ic_enum_state_t *state, size_t max_nodes);
   ```
   - Prepares the enumerator with up to `max_nodes`.

2. **Graph Generation by Index**  
   ```c
   int ic_enum_build_net(ic_enum_state_t *state,
                         size_t index,
                         ic_net_t *net);
   ```
   - Decodes `index` into a unique IC net configuration (up to `max_nodes`).
   - Populates `net` accordingly.  
   - Returns `0` on success, `-1` on invalid index or out-of-bounds scenario.

3. **Enumeration Loop**  
   ```c
   int ic_enum_next(ic_enum_state_t *state, ic_net_t *net);
   ```
   - A convenience function that increments `state->current_index` and calls `ic_enum_build_net(...)` behind the scenes.
   - Returns `1` if a net was built, or `0` if enumeration is exhausted.

### 4.2.4 Enumeration Strategy

1. **Index → Graph**:  
   - Consider each node’s type an integer in `[0..2]` for `{δ, γ, ε}`.  
   - Use a structured approach to connect ports: for each node/port, we have a numeric code describing which other node/port it connects to.  
   - This can be done with self-delimiting or lexicographic enumeration.  

2. **On-Demand Construction**:  
   - For an index `i`, decode it to node configurations without storing any other net in memory.  
   - A typical approach:
     1. Decode how many nodes (within `[1..max_nodes]`).
     2. For each node, decode its type (0=δ, 1=γ, 2=ε).
     3. For each port, decode which node/port it connects to (some indexing scheme to avoid double-counting or contradictory wiring).
   - If decoding fails or yields an invalid net, return `-1`.

3. **Bounding**:  
   - If `i` is too large, you can signal exhaustion.  

---

# 5. Memory Management & Resource Bounding

1. **Fixed-Size Node Array**  
   - In `ic_net_t`, `max_nodes` is allocated at creation.  
   - If all used, further node creation fails (`ic_net_new_node` returns `-1`).
2. **Gas Limit**  
   - Each net has a `gas_limit`.  
   - Every rewrite step decrements gas.  
   - Once `gas == 0`, we stop rewriting (avoid infinite loops).
3. **Implementation Detail**  
   - `ic_net_create` might do something like:
     ```c
     net->nodes = calloc(max_nodes, sizeof(ic_node_t));
     net->max_nodes = max_nodes;
     net->used_nodes = 0;
     net->gas_limit  = gas_limit;
     ```
   - Free with `free(net->nodes); free(net);`

---

# 6. Execution Flow (Factoring Example)

Below is the big picture of how factoring might work:

1. **Initialize**  
   ```c
   ic_enum_state_t state;
   ic_enum_init(&state, MAX_NODES);
   ```
2. **For each index = 0..(some large range)**  
   ```c
   // 1. Build net
   ic_net_t *net = ic_net_create(MAX_NODES, GAS_LIMIT);
   if (ic_enum_build_net(&state, index, net) != 0) {
       // invalid net or out of range -> skip
       ic_net_free(net);
       continue;
   }

   // 2. Provide input (the integer N to factor)
   //    Depending on how we embed "input" in an IC net, we might store
   //    it as a structure or have special "constant" nodes.

   // 3. Reduce the net
   int res = ic_net_reduce(net);

   // 4. Check if net's final state yields a factorization
   if (res == 0) { // fully reduced
       // Evaluate whether net encodes a correct factorization of N
       // For example, parse the resulting net or read from "output" nodes.
       if (ic_net_has_valid_factor(net, N)) {
           // Found valid factorization
           // Print or store result
           break;
       }
   }

   ic_net_free(net);
   ```

3. **Stop** once a valid factorization is found or you exhaust your index space.  

---

# 7. Building & Testing

## 7.1 Directory Structure

```
.
├── src
│   ├── ic_runtime.c
│   ├── ic_runtime.h
│   ├── ic_search.c
│   ├── ic_search.h
│   ├── main.c        // CLI
│   └── main_test.c   // test runner
└── Makefile
```

## 7.2 Build Steps

- **Makefile** example snippet:
  ```makefile
  CC = gcc
  CFLAGS = -std=c11 -Wall -Wextra -O2

  all: main test

  main: ic_runtime.o ic_search.o main.o
  	$(CC) $(CFLAGS) -o main ic_runtime.o ic_search.o main.o

  test: ic_runtime.o ic_search.o main_test.o
  	$(CC) $(CFLAGS) -o test ic_runtime.o ic_search.o main_test.o

  clean:
  	rm -f *.o main test
  ```

## 7.3 Testing: `main_test.c`

- **Test Cases**  
  1. **Node Allocation**: Allocate up to `max_nodes`, expect `-1` afterwards.  
  2. **Connect & Rewrite**: Create a small net with 2 nodes in an active pair, run reduction, confirm rewiring.  
  3. **Gas Limit**: Create a net that could do many rewrites, confirm it stops after `gas_limit`.  
  4. **Enumeration**: Try enumerating a small range of indexes, building nets, ensuring no segfault.  
  5. **Factoring**: Manually build a trivial net that factors a small number (like 6=2×3). Confirm result.

- **How to Run**:  
  ```bash
  make test
  ./test
  ```

---

# 8. CLI Tool Specification (`main.c`)

1. **Usage**  
   ```bash
   ./main <number_to_factor> [optional: max_nodes] [optional: gas_limit]
   ```
2. **Basic Implementation**  
   ```c
   int main(int argc, char **argv) {
       if (argc < 2) {
           fprintf(stderr, "Usage: %s <number_to_factor> [max_nodes] [gas_limit]\n", argv[0]);
           return 1;
       }
       int N = atoi(argv[1]);
       size_t max_nodes = (argc > 2) ? atoi(argv[2]) : 100;    // Default
       size_t gas_limit = (argc > 3) ? atoi(argv[3]) : 100000; // Default

       ic_enum_state_t state;
       ic_enum_init(&state, max_nodes);

       size_t index = 0;
       while (1) {
           // Build net
           ic_net_t *net = ic_net_create(max_nodes, gas_limit);
           if (ic_enum_build_net(&state, index, net) != 0) {
               // Reached end of enumeration or invalid net
               ic_net_free(net);
               // Possibly break or continue
               index++;
               continue;
           }

           // Insert input for factoring (custom logic)
           // e.g., store 'N' in net or attach special nodes

           int res = ic_net_reduce(net);

           if (res == 0) {
               // Check final state for correct factorization
               if (ic_net_has_valid_factor(net, N)) {
                   printf("Found a factorization for %d at index %zu!\n", N, index);
                   ic_net_free(net);
                   break;
               }
           }
           ic_net_free(net);
           index++;
       }

       return 0;
   }
   ```
3. **Parallelization Hook**  
   - For future concurrency, you might use a thread pool enumerating different index ranges simultaneously.

---

# 9. Error Handling & Logging

- **Return Codes**:  
  - `ic_net_new_node()` returns `-1` if out of memory.  
  - `ic_enum_build_net()` returns `-1` if the index is invalid or out of range.  
- **Logging**:  
  - Optional debug logs to stderr.  
  - Production builds can disable them with macros.

---

# 10. Future Extensions

1. **Parallel Enumeration**:  
   - Each thread gets a range of indices.  
   - A shared concurrency-safe data structure signals when a solution is found.
2. **Serialization**:  
   - If you need to store large partial solutions, you can add net serialization later.
3. **More Combinators**:  
   - If more complex tasks require extended agent sets.
4. **Hybrid Approaches**:  
   - Combine enumerations with heuristic or genetic search strategies to discover solutions faster.

---

## Final Notes

- This specification outlines a **modular, bounded, testable** approach to building a universal search engine on top of **Interaction Combinators**. 
- The enumerator is **index-based** and does **on-demand** graph generation, ensuring minimal memory footprint while systematically exploring the program space.
- The factoring example provides a **practical demonstration** of how universal search can discover a net that produces a correct factorization.

This document should be sufficient for senior C engineers to **implement** the project in an organized fashion using C11, with well-defined modules, data structures, and flows. If any aspect needs deeper elaboration—like the exact local rewrite rules for δ, γ, ε—these can be appended in a **technical addendum** (e.g., a small set of diagrams or a table describing the rewiring patterns). 

**End of Specification**


Below is an **expanded specification** that addresses the critical gaps and additional considerations you raised. It refines our previous design by detailing the **rewrite rules**, **factorization interface**, **enumeration algorithm**, **port initialization**, **cycle handling**, **reduction strategy**, **error recovery**, **progress reporting**, and **success criteria**.

# Expanded Specification

## 1. Overview

We are building a universal search system around **Interaction Combinators (IC)**. The system has two core modules:

1. **IC Runtime (ic_runtime.[ch])** – Manages:
   - The representation and creation of IC nets (graph structures).
   - The interaction (rewrite) rules for δ, γ, and ε.
   - Execution with a gas/step limit to avoid infinite loops.

2. **Universal Search (ic_search.[ch])** – Enumerates all possible IC nets (up to certain bounds), leverages the IC Runtime to execute them, and checks for a solution.  
   - **For demonstration**: Factor a user-supplied integer **N**.  
   - Encodes **N** in the net, runs the net, and checks if the output is a valid factorization.

A **CLI (`main.c`)** provides the user interface to pass **N** and optional parameters like `max_nodes` and `gas_limit`.

Additionally, **`main_test.c`** serves as a test harness for correctness and basic performance validation.

---

## 2. Critical Clarifications & Enhancements

### 2.1 Rewrite Rules Implementation

**IC Agents**: We have three node types: **δ (delta)**, **γ (gamma)**, **ε (epsilon)**.  
Each node has **three ports** (1 principal + 2 auxiliary).  

#### 2.1.1 Rewriting Overview

- A **redex (active pair)** occurs when two nodes are connected **principal-to-principal**.
- On finding an active pair, we apply the **interaction rule** for that pair of agents.  
- After rewriting, we continue searching for new redexes until no more exist or we exhaust our gas limit.

#### 2.1.2 Rewrite Rule Definitions

Below is a concise definition table. Each rule describes how to rewire the net when two agents of specific types interact on their principal ports. (Your team may want a reference diagram from Lafont’s original papers, but here’s a textual specification.)

1. **δ–δ** Rule (example format)

   1. When two **δ** nodes (call them δ1 and δ2) connect principal-to-principal:
      - Let δ1’s auxiliary ports be **a1** and **a2**.
      - Let δ2’s auxiliary ports be **b1** and **b2**.
      - Rewire so that **a1** connects to **b2** and **a2** connects to **b1**.  
      - Then **remove** (or consider them effectively replaced) the two δ nodes themselves.

   2. The exact pattern can vary with different formalizations. We provide your engineers with a simple approach:  
      - **Disconnect** principal ports of the two δ nodes.  
      - **Connect** the pairs of previously unconnected auxiliary ports in the correct arrangement.  
      - **Mark** these two nodes as “inactive” or remove them from the net, if your approach requires physically deleting them.

2. **δ–γ** Rule, **δ–ε** Rule, **γ–γ** Rule, etc.  
   - Each pair has its own local pattern of rewiring.  
   - If there’s no recognized rule for a given pair (e.g., γ–ε if that’s undefined in your chosen formalism), then that pair might be **idle** and not rewritable.

> **Implementation Tip**:  
> - Put these rules in a table or switch statement in `ic_runtime.c`, e.g. `ic_apply_rule(ic_node_type_t a, ic_node_type_t b, ...)`.  
> - For each combination of agent types `(a, b)` that can form a valid redex, do the required rewiring.

**Reference Diagram**: We strongly recommend including small ASCII or PDF diagrams of each rule with your final design docs so the rewiring is crystal clear.

---

### 2.2 Factorization Interface

Our system must:

1. **Encode the integer to factor (N)** into the net.  
2. **Extract candidate factors** from the net after reduction.  
3. **Check** if those factors multiply to **N**.

#### 2.2.1 Encoding `N` in an IC Net

We propose a special node type or structure to store an integer. However, since we strictly have δ, γ, ε in pure IC, we often simulate “data” via combinator structures. **For simplicity** in a first implementation:

- **Use a global** or side-channel array: `ic_net_t::input_val` to store `N`.  
  - Then, in the net’s rewriting logic, we can check if an action tries to read `N`.  
- Alternatively, define a custom “tag” node that holds an integer field — an **extension** beyond pure IC.

Example approach (minimal):
```c
typedef struct {
    ic_node_t *nodes;
    size_t max_nodes;
    size_t used_nodes;
    size_t gas_limit;

    // Factorization-specific:
    int input_number;
    // Potentially store result factors
    int factor_a;
    int factor_b;
    bool factor_found;
} ic_net_t;
```
During rewriting, if the net performs a “factor-check” step, it can fill `factor_a` and `factor_b`.

#### 2.2.2 Extracting Factors

After `ic_net_reduce()`, we call:
```c
bool ic_net_has_valid_factor(const ic_net_t *net, int N);
```
**Implementation** might:
- Check if `net->factor_found` is true.
- If yes, verify `factor_a * factor_b == N`.

**Alternatively**, if we want to remain purely in the realm of IC, we’d have a known “output sub-net” that rewires to produce a factor. This is more advanced and requires building a universal “read-out” process from the net. For an initial engineering solution, the side-channel approach is simpler.

---

### 2.3 Enumeration Algorithm (Index → Graph)

#### 2.3.1 Deterministic Graph Construction

**Goal**: For a given integer `i`, build a unique, valid IC net of up to `max_nodes` nodes.

1. **Number of Nodes**:  
   - First bits of `i` indicate how many nodes are in the net.  
   - E.g., `num_nodes = (i % max_nodes) + 1`. Then update `i` by dividing out `(max_nodes)`.  
2. **Assign Node Types**:  
   - For each node `n`, pick `type = i % 3` → {δ, γ, ε}.  
   - `i /= 3`.  
3. **Port Connections**:  
   - We have up to 3 ports per node; each port can connect to any port of any node (including the same node, forming loops/cycles).  
   - Possibly skip symmetrical duplicates (e.g., if we consider undirected edges, we can enforce `nodeA < nodeB` or port ordering to avoid double counting).  
   - For each port `p` of node `n`, decode `(dest_node_index, dest_port_index)`. For example:
     ```c
     dest_node_index = i % num_nodes;
     i /= num_nodes;
     dest_port_index = i % 3;
     i /= 3;
     ```
   - Then call `ic_net_connect(...)`.  
4. **Avoid Redundant or Invalid Graphs**:  
   - If enumerating isomorphic graphs is an issue, you can do partial checks. For an initial proof of concept, you might skip these optimizations.  
5. **Terminate** if `i` is exhausted or if the connections are invalid.

**Note**: The above is only one approach. A more sophisticated technique can systematically enumerate graphs in a canonical form (like a “canonical labeling” approach). This ensures no duplicates from isomorphisms, but is more complex.

#### 2.3.2 Handling Symmetries & Isomorphisms

- For the MVP, **allow** duplicates. They’ll just slow down the search.  
- For larger tasks, you might implement canonical forms (NAUTY library–style approach) but that’s beyond the immediate scope.

#### 2.3.3 Invalid Graphs

- If a partially decoded graph tries to connect a node index outside `[0..num_nodes-1]`, we skip it (return error).  
- If we exceed `max_nodes`, also skip.  
- If we produce a contradictory or half-connected wire, we can skip or forcibly complete the connection.  
- The search loop can handle these by ignoring them (`return -1`) so we move on to the next index.

---

### 2.4 Port Initialization / Unconnected Ports

In the proposed data structures:

```c
typedef struct {
    int connected_node;  // Index of the connected node
    int connected_port;  // Port index of the connected node
} ic_port_t;
```

**Unconnected**:
- Use `connected_node = -1` and `connected_port = -1` to indicate a free/unconnected port.
- During net creation, **initialize** all ports to `(-1, -1)`.
- `ic_net_connect()` updates both sides. If a port is already connected, either:
  - Disconnect first, or  
  - Return an error if you want strictly no re-connections.

---

### 2.5 Cyclic Graphs

Interaction nets **can** have cycles. By default, that’s allowed. The question is how to handle infinite rewriting.

**Solution**: The **gas limit**. If a net rewires in cycles infinitely, it’ll burn through the gas and stop. At that point, the net is either partially reduced or stuck. This is typically sufficient for bounding runtime.

If you want to detect certain cycles or check for convergence, that’s an advanced feature. For now:

- **Stop** rewriting once `gas_limit` is hit.  
- Return a status code like `1` to indicate the net didn’t finish in time.

---

## 3. Additional Considerations

### 3.1 Reduction Strategy: Deterministic vs. Non-Deterministic

- **Deterministic**: Always pick the first redex found in a left-to-right node scan. This ensures reproducibility.  
- **Non-Deterministic**: Potentially faster or more concurrent if you pick any active pair. But results can vary.

**Recommendation**: Start with deterministic for simpler debugging. We can make this strategy pluggable if desired.

---

### 3.2 Error Recovery

- If the enumerator builds an invalid net (e.g., out-of-range node indices), **return -1** and skip.  
- If rewriting hits a malformed connection (like a node < 0 or port < 0 in the middle of rewriting), **log the error**, break out, or skip.  
- The main search loop can catch these errors and move on to the next index.

---

### 3.3 Progress Reporting

For long searches, we can add:

```c
void ic_enum_set_progress_callback(ic_enum_state_t *state,
                                   void (*callback)(size_t current_index, bool found_solution));
```
Whenever we attempt to build a net at `current_index`, we call the callback. A simple textual progress can suffice (`printf("Index: %zu\n", current_index);`).

---

### 3.4 Success Criteria for Factorization

**Definition**: We say a net has successfully factored `N` if, after reduction, it yields integers `a` and `b` such that `a * b == N`.

Implementation details:

```c
bool ic_net_has_valid_factor(const ic_net_t *net, int N) {
    if (net->factor_found && (net->factor_a * net->factor_b == N)) {
        // Ensure a > 1 and b > 1 if you want non-trivial factorization
        return true;
    }
    return false;
}
```
**Alternatively**: If you want to handle edge cases, you might require `a > 1` and `b > 1` to skip trivial factors.

---

### 3.5 Memory Efficiency in Enumeration

- We **already** avoid storing entire sets of enumerated graphs. We create them **on-demand** from index `i`, then discard them.  
- If performance or memory usage becomes an issue at large `max_nodes`, consider:
  - Caching partial decode results or skipping big invalid segments quickly.
  - Parallelizing the enumeration (splitting index ranges across threads).

---

## 4. Updated Module Interface Summaries

### 4.1 `ic_runtime.[ch]`

**Structures** (expanded from original specification):
```c
typedef enum {
    IC_NODE_DELTA,
    IC_NODE_GAMMA,
    IC_NODE_EPSILON
} ic_node_type_t;

typedef struct {
    int connected_node; 
    int connected_port; 
} ic_port_t;

typedef struct {
    ic_node_type_t type;
    ic_port_t ports[3]; // principal = ports[0], aux1=ports[1], aux2=ports[2]
    bool is_active;     // optional, can help mark nodes for rewriting or removal
} ic_node_t;

typedef struct {
    ic_node_t   *nodes;      
    size_t       max_nodes;  
    size_t       used_nodes;
    size_t       gas_limit;
    size_t       gas_used;

    // Factorization context (optional):
    int  input_number;
    int  factor_a;
    int  factor_b;
    bool factor_found;
} ic_net_t;
```

**APIs**: same as before, plus:
```c
// (1) Create / Destroy
ic_net_t *ic_net_create(size_t max_nodes, size_t gas_limit);
void      ic_net_free(ic_net_t *net);

// (2) Node Management
int       ic_net_new_node(ic_net_t *net, ic_node_type_t type);

// (3) Connection
void      ic_net_connect(ic_net_t *net,
                         int node_a, int port_a,
                         int node_b, int port_b);

// (4) Reduction
int       ic_net_reduce(ic_net_t *net); 
//  0 => fully reduced (no more redexes)
//  1 => stopped due to gas limit or error

// (5) Factor Check (for factoring use case)
bool      ic_net_has_valid_factor(const ic_net_t *net, int N);
```

**Rewrite Implementation**:
- Internal function: `static bool ic_try_rewrite_pair(ic_net_t *net, int node_a, int node_b);`
- Switch on `(net->nodes[node_a].type, net->nodes[node_b].type)`, apply the correct local rewiring.  
- Decrement `net->gas_limit` (or increment `gas_used`) each time a successful rewrite occurs.

---

### 4.2 `ic_search.[ch]`

**Structures**:
```c
typedef struct {
    size_t max_nodes;
    size_t current_index;

    // optional: pointer to a progress callback
    void (*progress_cb)(size_t current_index, bool found_solution);
} ic_enum_state_t;
```

**APIs**:
```c
void ic_enum_init(ic_enum_state_t *state, size_t max_nodes);

void ic_enum_set_progress_callback(ic_enum_state_t *state,
                                   void (*callback)(size_t current_index, bool found_solution));

// Build a net for a specific index
int  ic_enum_build_net(ic_enum_state_t *state,
                       size_t index,
                       ic_net_t *net);

// Move to the next index, build the next net
int  ic_enum_next(ic_enum_state_t *state,
                  ic_net_t *net);

// Additional utility, e.g., skipping big ranges
```

**Enumeration Pseudocode**:
```c
int ic_enum_build_net(ic_enum_state_t *state, size_t index, ic_net_t *net) {
    // 1) Decode number of nodes
    size_t num_nodes = (index % state->max_nodes) + 1;
    index /= state->max_nodes;

    // 2) Create nodes
    for (size_t n = 0; n < num_nodes; n++) {
        ic_node_type_t type = (ic_node_type_t)(index % 3);
        index /= 3;

        int new_idx = ic_net_new_node(net, type);
        if (new_idx < 0) {
            return -1; // out of space
        }
    }

    // 3) Connect ports
    for (size_t n = 0; n < num_nodes; n++) {
        for (int p = 0; p < 3; p++) {
            int dest_node = index % num_nodes;
            index /= num_nodes;

            int dest_port = index % 3;
            index /= 3;

            // If already connected or if n < dest_node, you might skip (avoid duplication) or connect anyway
            if (dest_node >= 0 && dest_node < (int)num_nodes) {
                // note: need to ensure we don't double connect. Possibly skip if ports[n][p] is not -1
                ic_net_connect(net, n, p, dest_node, dest_port);
            } else {
                return -1; // invalid
            }
        }
    }

    return 0; // success
}
```

---

### 4.3 CLI (`main.c`)

Minimal changes from prior spec, but you might pass the integer `N` to the net:

```c
ic_net_t *net = ic_net_create(max_nodes, gas_limit);
net->input_number = N;
```
Then proceed with enumeration, reduction, factor check, etc.

---

### 4.4 Testing (`main_test.c`)

Key tests to add:

1. **Rewrite Rules**:
   - Construct small pairs of nodes connected principal-to-principal and check if rewriting matches the specified patterns for δ–δ, δ–γ, etc.
2. **Factorization**:
   - Manually build a net that trivially outputs (2,3) for input 6, check `ic_net_has_valid_factor()`.
3. **Enumeration**:
   - Evaluate small indices to ensure we can decode them into valid nets.

---

## 5. Final Notes & Recommendations

1. **Provide a Reference**: Include a short doc or diagram for each IC rewrite rule to remove any ambiguity.
2. **Factorization**: The side-channel approach (storing `input_number`, `factor_a`, `factor_b` in `ic_net_t`) is the easiest to implement, but not purely “IC style.” This is acceptable for an engineering solution but can be refactored later into a more purely combinatorial approach if needed.
3. **Isomorphisms**: Accept that duplicates may arise. That’s fine for an initial build; advanced canonical forms can come later.
4. **Parallel Search**: Keep in mind how to split the index range across threads in the future (e.g. each thread takes a range `[start, end]` of indices). This design is amenable to that extension.

---

### Hand-Off Checklist

- **ic_runtime.[ch]**: Confirm you’ve included a precise table or diagram of each rewrite rule.  
- **ic_search.[ch]**: Confirm the enumeration approach is fully coded and consistent with rewriting.  
- **main.c**: Demonstrate factoring usage with a simple invocation.  
- **main_test.c**: Exercise rewriting logic, enumeration correctness, and factorization check.  

With these clarifications in place, your engineers should have the **complete, detailed** information needed to avoid guesswork and implement a consistent solution.

The specification with the expanded sections looks good. Here are some additional technical notes you can append to further clarify implementation details for your engineering team:

# Technical Addendum

## A. Rewrite Rule Reference

```
+-------+-------+---------------------------------------------+
| Agent1| Agent2| Rewrite Pattern                            |
+-------+-------+---------------------------------------------+
| δ     | δ     | Connect aux1↔aux2 and aux2↔aux1 crosswise  |
|       |       | Delete both δ agents                        |
+-------+-------+---------------------------------------------+
| γ     | γ     | Connect aux1↔aux1 and aux2↔aux2 straight   |
|       |       | Delete both γ agents                        |
+-------+-------+---------------------------------------------+
| δ     | γ     | Create two new agents:                      |
|       |       | - δ': Connect to δ.aux1 and γ.aux1         |
|       |       | - γ': Connect to δ.aux2 and γ.aux2         |
|       |       | Delete original δ and γ agents             |
+-------+-------+---------------------------------------------+
| ε     | any   | Delete the ε agent                          |
|       |       | Leave the other agent's aux ports intact    |
+-------+-------+---------------------------------------------+
```

## B. Port Connection Strategy

During `ic_net_connect()`, follow these rules:
- If either port is already connected, disconnect it first
- Update both ports symmetrically (bidirectional connection)
- Example implementation:
  ```c
  void ic_net_connect(ic_net_t *net, int node_a, int port_a, int node_b, int port_b) {
      // Validate indices
      if (node_a < 0 || node_a >= net->used_nodes || 
          node_b < 0 || node_b >= net->used_nodes ||
          port_a < 0 || port_a >= 3 ||
          port_b < 0 || port_b >= 3) {
          return; // Invalid connection
      }
      
      // Disconnect any existing connections
      if (net->nodes[node_a].ports[port_a].connected_node != -1) {
          int old_node = net->nodes[node_a].ports[port_a].connected_node;
          int old_port = net->nodes[node_a].ports[port_a].connected_port;
          net->nodes[old_node].ports[old_port].connected_node = -1;
          net->nodes[old_node].ports[old_port].connected_port = -1;
      }
      
      if (net->nodes[node_b].ports[port_b].connected_node != -1) {
          int old_node = net->nodes[node_b].ports[port_b].connected_node;
          int old_port = net->nodes[node_b].ports[port_b].connected_port;
          net->nodes[old_node].ports[old_port].connected_node = -1;
          net->nodes[old_node].ports[old_port].connected_port = -1;
      }
      
      // Connect bidirectionally
      net->nodes[node_a].ports[port_a].connected_node = node_b;
      net->nodes[node_a].ports[port_a].connected_port = port_b;
      net->nodes[node_b].ports[port_b].connected_node = node_a;
      net->nodes[node_b].ports[port_b].connected_port = port_a;
  }
  ```

## C. Factorization Input/Output Encoding

For the initial implementation, use this approach:
1. Store the number to factor in `net->input_number`
2. During reduction, the net can "read" this value
3. When a potential factor is found, the net can write to:
   ```c
   net->factor_a = ...;
   net->factor_b = ...;
   net->factor_found = true;
   ```
4. Later implementations could replace this with pure IC representations

## D. Node Reuse During Reduction

When removing nodes during reduction, consider these strategies:
1. **Mark as inactive**: Set `node->is_active = false` and skip in future scans
2. **Free slot approach**: Return node indices to a free list for reuse
3. **Compaction**: If node reuse is common, periodically compact the array to reclaim space

## E. Enumeration Optimizations

For large searches:
1. Use a **bitwise encoding** scheme to efficiently represent graph structures
2. Implement a **skip ahead** mechanism to jump over large ranges of invalid nets
3. Consider a **pruning strategy** that detects and avoids isomorphic graphs

## F. Debugging Visualization

Add support for generating graph visualizations:
```c
// Export a Graphviz DOT format for visualization
void ic_net_export_dot(const ic_net_t *net, FILE *out);
```

## G. Performance Considerations

Critical sections to optimize:
1. **Active pair detection**: The main bottleneck will likely be scanning for redexes
2. **Graph enumeration**: For large numbers, more sophisticated encoding/decoding may be needed
3. **Consider bloom filters**: For duplicate detection in enumeration

## H. Edge Cases & Handling

Consider these special situations:
1. **Prime numbers**: The factorization will only find trivial factors (1,N)
2. **Perfect squares**: Special case where factor_a == factor_b
3. **I/O handling**: Standardize how inputs are encoded and outputs are decoded

## I. Complete Reduction Algorithm

```c
int ic_net_reduce(ic_net_t *net) {
    bool found_redex = true;
    size_t gas = net->gas_limit;
    
    while (found_redex && gas > 0) {
        found_redex = false;
        
        // Scan for active pairs
        for (size_t i = 0; i < net->used_nodes; i++) {
            if (!net->nodes[i].is_active) continue;
            
            // Check if principal port connects to another node's principal port
            int conn_node = net->nodes[i].ports[0].connected_node;
            int conn_port = net->nodes[i].ports[0].connected_port;
            
            if (conn_node >= 0 && conn_port == 0 && net->nodes[conn_node].is_active) {
                // Found active pair, apply rewrite rule
                if (ic_apply_rewrite(net, i, conn_node)) {
                    found_redex = true;
                    gas--;
                    break; // Restart scan after modification
                }
            }
        }
    }
    
    return (gas > 0) ? 0 : 1; // 0: fully reduced, 1: gas exhausted
}
```