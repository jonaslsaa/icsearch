#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ic_runtime.h"
#include "ic_search.h"

// Callback for reporting search progress
void progress_callback(size_t current_index, bool found_solution) {
    if (found_solution) {
        printf("Found solution at index %zu!\n", current_index);
    } else {
        printf("Searched through %zu indices...\n", current_index);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_to_factor> [max_nodes] [gas_limit]\n", argv[0]);
        return 1;
    }
    
    // Parse arguments
    int N = atoi(argv[1]);
    size_t max_nodes = (argc > 2) ? atoi(argv[2]) : 100;
    size_t gas_limit = (argc > 3) ? atoi(argv[3]) : 100000;
    
    if (N <= 1) {
        fprintf(stderr, "The number to factor must be greater than 1\n");
        return 1;
    }
    
    printf("Searching for a factorization of %d with max_nodes=%zu and gas_limit=%zu\n",
           N, max_nodes, gas_limit);
    
    // Initialize the enumeration state
    ic_enum_state_t state;
    ic_enum_init(&state, max_nodes);
    ic_enum_set_progress_callback(&state, progress_callback);
    
    // Start timing
    clock_t start = clock();
    
    // Run the search
    int solution_index = ic_search_factor(&state, N, max_nodes, gas_limit);
    
    // End timing
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    if (solution_index >= 0) {
        printf("\nSuccess! Found a factorization for %d at index %d\n", N, solution_index);
        
        // Reconstruct the solution net for demonstration
        ic_net_t *solution_net = ic_net_create(max_nodes, gas_limit);
        solution_net->input_number = N;
        
        if (ic_enum_build_net(&state, solution_index, solution_net) == 0) {
            // Reduce it again to find the factors
            ic_net_reduce(solution_net);
            
            // Print the factors
            if (ic_net_has_valid_factor(solution_net, N)) {
                printf("Factors: %d * %d = %d\n", 
                       solution_net->factor_a, solution_net->factor_b, N);
            }
            
            // Print the net
            ic_net_print(solution_net);
            
            // Export dot file for visualization
            FILE *dot_file = fopen("solution.dot", "w");
            if (dot_file) {
                ic_net_export_dot(solution_net, dot_file);
                fclose(dot_file);
                printf("Graph visualization saved to solution.dot\n");
                printf("You can visualize it with: dot -Tpng solution.dot -o solution.png\n");
            }
        }
        
        ic_net_free(solution_net);
    } else {
        printf("\nFailed to find a factorization for %d\n", N);
    }
    
    printf("\nSearch completed in %.2f seconds\n", elapsed);
    
    return (solution_index >= 0) ? 0 : 1;
}