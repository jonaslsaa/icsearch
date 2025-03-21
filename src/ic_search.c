#include "ic_search.h"
#include <stdlib.h>
#include <stdio.h>

void ic_enum_init(ic_enum_state_t *state, size_t max_nodes) {
    if (!state) return;
    
    state->max_nodes = max_nodes;
    state->current_index = 0;
    state->progress_cb = NULL;
}

void ic_enum_set_progress_callback(ic_enum_state_t *state,
                                  void (*callback)(size_t current_index, bool found_solution)) {
    if (!state) return;
    state->progress_cb = callback;
}

int ic_enum_build_net(ic_enum_state_t *state, size_t index, ic_net_t *net) {
    if (!state || !net) return -1;
    
    // Fast reset - just setting used_nodes to 0 is most important
    net->used_nodes = 0;
    net->gas_used = 0;
    net->factor_found = false;
    
    // Limit net size for performance
    // Small nets are more likely to have useful computational behavior
    size_t max_net_size = 10; // Small nets are faster to evaluate
    size_t num_nodes = 3 + (index % max_net_size);
    
    // Extract some randomization bits from the index
    unsigned int pattern = index / max_net_size;
    
    // Ensure we have at least one active pair (principal-principal connection)
    bool has_active_pair = false;
    
    // Directly create a delta-gamma active pair for factorization
    int delta_node = ic_net_new_node(net, IC_NODE_DELTA);
    int gamma_node = ic_net_new_node(net, IC_NODE_GAMMA);
    
    if (delta_node < 0 || gamma_node < 0) {
        return -1; // Out of space
    }
    
    // Connect principal ports to create active pair - guarantees computation
    ic_net_connect(net, delta_node, 0, gamma_node, 0);
    has_active_pair = true;
    
    // Generate the rest of the nodes with fast pattern-based distribution
    for (size_t n = 2; n < num_nodes; n++) {
        // Fast node type selection based on bit pattern
        ic_node_type_t type;
        unsigned int bit = (pattern >> (n % 16)) & 0x3; // Use 2 bits
        
        if (bit == 0) {
            type = IC_NODE_DELTA;
        } else if (bit == 1) {
            type = IC_NODE_GAMMA;
        } else {
            type = IC_NODE_EPSILON;
        }
        
        int new_idx = ic_net_new_node(net, type);
        if (new_idx < 0) {
            return -1; // Out of space
        }
    }
    
    // Fast bit-pattern based connection scheme
    // Form a cycle to ensure all ports are connected
    for (size_t i = 0; i < net->used_nodes; i++) {
        // Connect auxiliary ports to next/prev nodes to form a ring
        size_t next = (i + 1) % net->used_nodes;
        size_t prev = (i + net->used_nodes - 1) % net->used_nodes;
        
        // Skip principal ports that are already connected in active pair
        if (i == 0 || i == 1) {
            // Connect auxiliary ports only
            ic_net_connect(net, i, 1, next, 2);
            ic_net_connect(net, i, 2, prev, 1);
        } else {
            // For other nodes, connect all ports
            ic_net_connect(net, i, 0, (i + 2) % net->used_nodes, 0);
            ic_net_connect(net, i, 1, next, 2);
            ic_net_connect(net, i, 2, prev, 1);
        }
    }
    
    // We know all ports are connected and we have an active pair
    return 0;
}

int ic_enum_next(ic_enum_state_t *state, ic_net_t *net) {
    if (!state || !net) return 0;
    
    // With our optimized algorithm, almost all builds succeed
    // So we can simply increment and try once
    if (ic_enum_build_net(state, state->current_index, net) == 0) {
        state->current_index++;
        return 1;
    }
    
    // If build fails, we're likely at the end of our space
    state->current_index++;
    return 0;
}

int ic_search_factor(ic_enum_state_t *state, int N, size_t max_nodes, size_t gas_limit) {
    if (!state || N <= 1) return -1;
    
    // Report progress every chunk_size attempts
    const size_t progress_chunk = 1000;
    size_t current_chunk = 0;
    
    // Create a net for evaluation
    ic_net_t *net = ic_net_create(max_nodes, gas_limit);
    if (!net) return -1;
    
    // Set the input number to factor
    net->input_number = N;
    
    // Track the solution index
    int solution_index = -1;
    
    // Loop until we find a solution or exhaust the search space
    while (solution_index == -1) {
        // Try to build next valid net
        if (ic_enum_next(state, net) == 0) {
            break; // Exhausted search space
        }
        
        // Run the reduction
        ic_net_reduce(net);
        
        // Check if we found a valid factorization
        if (ic_net_has_valid_factor(net, N)) {
            solution_index = state->current_index - 1;
            
            // Report progress with solution
            if (state->progress_cb) {
                state->progress_cb(state->current_index - 1, true);
            }
            
            break;
        }
        
        // Report progress periodically
        if (state->progress_cb && (state->current_index / progress_chunk > current_chunk)) {
            current_chunk = state->current_index / progress_chunk;
            state->progress_cb(state->current_index - 1, false);
        }
    }
    
    // Clean up
    ic_net_free(net);
    
    return solution_index;
}