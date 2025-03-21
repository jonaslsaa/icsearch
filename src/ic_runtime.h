#ifndef IC_RUNTIME_H
#define IC_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Types of IC nodes (δ, γ, ε)
 */
typedef enum {
    IC_NODE_DELTA,   // δ
    IC_NODE_GAMMA,   // γ
    IC_NODE_EPSILON  // ε
} ic_node_type_t;

/**
 * Connection to another port
 */
typedef struct {
    int connected_node;  // Index of the connected node (-1 if not connected)
    int connected_port;  // Which port on that node (-1 if not connected)
} ic_port_t;

/**
 * Node in an interaction combinator graph
 */
typedef struct {
    ic_node_type_t type;
    ic_port_t ports[3];  // principal = ports[0], aux1 = ports[1], aux2 = ports[2]
    bool is_active;      // Used during rewriting
} ic_node_t;

/**
 * Interaction Combinator network/graph
 */
typedef struct {
    ic_node_t *nodes;     // Array of nodes in this net
    size_t max_nodes;     // Capacity (bound on total nodes)
    size_t used_nodes;    // How many nodes currently allocated
    size_t gas_limit;     // Maximum rewrite steps
    size_t gas_used;      // How many rewrite steps used so far

    // Factorization context
    int input_number;
    int factor_a;
    int factor_b;
    bool factor_found;
} ic_net_t;

/**
 * Create a new IC net with the given maximum nodes and gas limit
 */
ic_net_t *ic_net_create(size_t max_nodes, size_t gas_limit);

/**
 * Free an IC net
 */
void ic_net_free(ic_net_t *net);

/**
 * Create a new node in the net of the given type
 * @return The index of the new node, or -1 if no space
 */
int ic_net_new_node(ic_net_t *net, ic_node_type_t type);

/**
 * Connect two node ports together
 */
void ic_net_connect(ic_net_t *net, int node_a, int port_a, int node_b, int port_b);

/**
 * Run the rewriting until no active pairs remain or gas is exhausted
 * @return 0 if fully reduced, 1 if stopped due to gas limit
 */
int ic_net_reduce(ic_net_t *net);

/**
 * Check if the net has found a valid factorization of N
 */
bool ic_net_has_valid_factor(const ic_net_t *net, int N);

/**
 * Print the net (for debugging)
 */
void ic_net_print(const ic_net_t *net);

/**
 * Get the number of used nodes
 */
size_t ic_net_get_used_nodes(const ic_net_t *net);

/**
 * Export the net to dot format for visualization
 */
void ic_net_export_dot(const ic_net_t *net, FILE *out);

#endif /* IC_RUNTIME_H */