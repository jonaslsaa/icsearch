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
    
    // Reset the net
    for (size_t i = 0; i < net->used_nodes; i++) {
        net->nodes[i].is_active = false;
    }
    net->used_nodes = 0;
    net->gas_used = 0;
    net->factor_found = false;
    
    // 1. Decode number of nodes - keep this simple and fast
    // For factorization, we need at least 3 nodes (input, operation, output)
    size_t num_nodes = 3 + (index % (state->max_nodes - 3));
    size_t remaining_index = index / (state->max_nodes - 3);
    
    // 2. Create nodes - mix of delta and gamma nodes
    // This ensures a balance of computational and connection nodes
    for (size_t n = 0; n < num_nodes; n++) {
        // Simple distribution: alternate types with some epsilon nodes
        ic_node_type_t type;
        if (n % 5 == 0) {
            type = IC_NODE_EPSILON; // Some erasure nodes
        } else if (n % 2 == 0) {
            type = IC_NODE_DELTA;
        } else {
            type = IC_NODE_GAMMA;
        }
        
        int new_idx = ic_net_new_node(net, type);
        if (new_idx < 0) {
            return -1; // Out of space
        }
    }
    
    // 3. Connect ports - keep this fast and simple
    for (size_t n = 0; n < num_nodes; n++) {
        for (int p = 0; p < 3; p++) {
            // Skip if this port is already connected
            if (net->nodes[n].ports[p].connected_node != -1) {
                continue;
            }
            
            // Simple connection strategy: connect to next node modulo index
            size_t dest_node = (n + 1 + (remaining_index % num_nodes)) % num_nodes;
            remaining_index /= num_nodes;
            
            // Select a port based on remaining index
            int dest_port = remaining_index % 3;
            remaining_index /= 3;
            
            // Skip if this would cause a duplicate connection
            if (net->nodes[dest_node].ports[dest_port].connected_node != -1) {
                // Just find the next available port on this node
                bool found = false;
                for (int q = 0; q < 3; q++) {
                    if (net->nodes[dest_node].ports[q].connected_node == -1) {
                        dest_port = q;
                        found = true;
                        break;
                    }
                }
                
                // If no free port, try the next node
                if (!found) {
                    for (size_t m = 0; m < num_nodes; m++) {
                        if (m == n) continue; // Skip self
                        
                        for (int q = 0; q < 3; q++) {
                            if (net->nodes[m].ports[q].connected_node == -1) {
                                dest_node = m;
                                dest_port = q;
                                found = true;
                                break;
                            }
                        }
                        if (found) break;
                    }
                }
                
                // If still not found, this might be an invalid net configuration
                if (!found) {
                    continue;
                }
            }
            
            // Skip self-connections to principal port (can't connect principal to itself)
            if (n == dest_node && p == 0 && dest_port == 0) {
                continue;
            }
            
            // Connect the ports
            ic_net_connect(net, n, p, dest_node, dest_port);
        }
    }
    
    // Ensure all ports are connected - any unconnected ports means invalid net
    bool has_unconnected = false;
    
    for (size_t n = 0; n < num_nodes; n++) {
        for (int p = 0; p < 3; p++) {
            if (net->nodes[n].ports[p].connected_node == -1) {
                has_unconnected = true;
                
                // Try to connect to any available port
                bool connected = false;
                for (size_t m = 0; m < num_nodes && !connected; m++) {
                    if (m == n) continue; // Avoid self for simplicity
                    
                    for (int q = 0; q < 3 && !connected; q++) {
                        if (net->nodes[m].ports[q].connected_node == -1) {
                            ic_net_connect(net, n, p, m, q);
                            connected = true;
                        }
                    }
                }
                
                if (!connected) {
                    // Couldn't connect - invalid net
                    return -1;
                }
            }
        }
    }
    
    // Simple validity check - at least one active pair to start computation
    bool has_active_pair = false;
    for (size_t i = 0; i < num_nodes && !has_active_pair; i++) {
        int conn_node = net->nodes[i].ports[0].connected_node;
        if (conn_node >= 0 && net->nodes[conn_node].ports[0].connected_node == (int)i) {
            has_active_pair = true;
        }
    }
    
    // Minimum complexity check - for factorization we need some active computation
    if (!has_active_pair) {
        return -1;
    }
    
    return 0;
}

int ic_enum_next(ic_enum_state_t *state, ic_net_t *net) {
    if (!state || !net) return 0;
    
    size_t max_attempts = 1000; // Avoid infinite loop
    size_t attempts = 0;
    
    while (attempts < max_attempts) {
        if (ic_enum_build_net(state, state->current_index, net) == 0) {
            state->current_index++;
            return 1;
        }
        
        state->current_index++;
        attempts++;
    }
    
    return 0; // Exhausted or too many failures
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