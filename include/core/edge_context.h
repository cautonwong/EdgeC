#ifndef EDGE_CONTEXT_H
#define EDGE_CONTEXT_H

#include <uv.h>
#include "shmring.h"
#include "algorithm_sum.h"
#include "persistence_mgr.h"
#include "northbound_mgr.h"
#include "sys_supervisor.h"
#include "io_executor.h"
#include "arbiter.h"
#include "bus_orchestrator.h"
#include "profile_loader.h"
#include "datahub.h"
#include "core/net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

// The main application context struct
typedef struct {
    uv_loop_t *loop;
    
    // Core Services
    arbiter_t *arbiter;
    datahub_t data_hub;
    algo_sum_t algo_sum;
    persistence_mgr_t storage;
    northbound_mgr_t cloud;
    sys_supervisor_t supervisor;
    
    // Memory Subsystem
    net_buf_pool_t packet_pool; // For IO packets (small)
    net_buf_pool_t blob_pool;   // For large blobs (optional)

    // Configuration
    device_config_t *config;
    
    // Timers & Handles
    uv_timer_t poll_timer;
    uv_signal_t sig_int;
    uv_signal_t sig_term;

    // Flag to signal shutdown
    int shutting_down;

} edge_ctx_t;

/**
 * @brief Initializes the master EdgeOS context.
 */
int edge_ctx_init(edge_ctx_t *ctx, const char *config_file);

/**
 * @brief Starts the main event loop.
 */
int edge_ctx_start(edge_ctx_t *ctx);

/**
 * @brief Signals the application to stop and cleans up resources.
 */
void edge_ctx_stop(edge_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // EDGE_CONTEXT_H
