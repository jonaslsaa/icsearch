#ifndef IC_SEARCH_H
#define IC_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include "ic_runtime.h"

/**
 * State for enumerating IC nets
 */
typedef struct {
    size_t max_nodes;
    size_t current_index;

    // Progress callback
    void (*progress_cb)(size_t current_index, bool found_solution);
} ic_enum_state_t;

/**
 * Initialize an enumeration state
 */
void ic_enum_init(ic_enum_state_t *state, size_t max_nodes);

/**
 * Set a progress callback
 */
void ic_enum_set_progress_callback(ic_enum_state_t *state,
                                  void (*callback)(size_t current_index, bool found_solution));

/**
 * Build a net for a specific index
 * @return 0 on success, -1 if invalid/out of range
 */
int ic_enum_build_net(ic_enum_state_t *state, size_t index, ic_net_t *net);

/**
 * Version of ic_enum_build_net that doesn't rely on state
 * Used in parallel search to avoid state dependencies
 */
int ic_enum_build_net_compatible(ic_enum_state_t *state, size_t index, ic_net_t *net);

/**
 * Build the next net and increment the index
 * @return 1 if a net was built, 0 if enumeration is exhausted
 */
int ic_enum_next(ic_enum_state_t *state, ic_net_t *net);

/**
 * Run the search for a net that factors the given number
 * @return The index of the solution, or -1 if none found
 */
int ic_search_factor(ic_enum_state_t *state, int N, size_t max_nodes, size_t gas_limit);

#endif /* IC_SEARCH_H */