#ifndef EDGE_CONTEXT_H
#define EDGE_CONTEXT_H

#include <uv.h>
#include "arbiter.h"
#include "datahub.h"
// Other module headers will be included here as they are created/refactored

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for module types
typedef struct algo_sum_s algo_sum_t;
// Add other forward declarations as needed

// The main application context struct
typedef struct {
    uv_loop_t loop;
    
    // Module handles will be centralized here
    arbiter_t *arbiter;
    datahub_t data_hub;
    
    // Example of an algorithm service handle
    algo_sum_t *algorithm_sum;

    // Flag to signal shutdown
    int shutting_down;

} edge_ctx_t;

/**
 * @brief Initializes the master EdgeOS context.
 * 
 * This function initializes the main event loop and all core modules.
 * @param ctx The context object to initialize.
 * @return 0 on success, negative error code on failure.
 */
int edge_ctx_init(edge_ctx_t *ctx);

/**
 * @brief Starts the main event loop.
 * 
 * This function enters the main uv_run loop and will block until the
 * application is stopped.
 * @param ctx The application context.
 * @return 0 on success, negative error code on failure.
 */
int edge_ctx_start(edge_ctx_t *ctx);

/**
 * @brief Signals the application to stop and cleans up resources.
 * 
 * This function can be called from a signal handler or another thread
 * to gracefully shut down the application.
 * @param ctx The application context.
 */
void edge_ctx_stop(edge_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // EDGE_CONTEXT_H
