#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "shmring.h"
#include "algorithm_sum.h"
#include "sqlite_service.h"
#include "modbus_protocol.h"
#include "edge_version.h"

// Global Context
typedef struct {
    uv_loop_t *loop;
    shm_ring_t data_hub;
    algo_sum_t algo_sum;
    uv_signal_t sig_term;
    uv_timer_t stats_timer;
} app_ctx_t;

static app_ctx_t g_ctx;

// Required by modbus_protocol.c mock
shm_ring_t *global_ring = NULL; 

// --- Callbacks ---

static void on_sql_init_done(int rc, void *arg) {
    if (rc != 0) {
        fprintf(stderr, "DB Init Failed: %d\n", rc);
        exit(1);
    }
    printf("[Main] Database initialized (Async).\n");
}

static void on_stats_timer(uv_timer_t *handle) {
    app_ctx_t *ctx = handle->data;
    printf("[Main] Tick... Algo Sum: %.2f\n", ctx->algo_sum.sum);
    
    // Simulate triggering a Modbus read every tick
    modbus_read_request(101);
}

static void on_signal(uv_signal_t *handle, int signum) {
    printf("\n[Main] Caught signal %d, shutting down...\n", signum);
    uv_stop(handle->loop);
}

int main(void) {
    printf("Starting EdgeOS Core %s...\n", edge_get_version());

    g_ctx.loop = uv_default_loop();
    global_ring = &g_ctx.data_hub; // Link global pointer for protocol mock

    // 1. Initialize Shared Memory Ring
    // Try to open existing first, or create new
    if (shmring_open(&g_ctx.data_hub, "/edgeos_hub") != 0) {
        printf("[Main] Creating new shm_ring...\n");
        if (shmring_create(&g_ctx.data_hub, "/edgeos_hub", 128) != 0) {
            fprintf(stderr, "Failed to create shm_ring\n");
            return 1;
        }
    }

    // 2. Initialize SQLite (Async)
    // We pass the loop because our new API requires it
    sqlite_exec_async(g_ctx.loop, 
        "CREATE TABLE IF NOT EXISTS logs (id INTEGER PRIMARY KEY, msg TEXT);", 
        on_sql_init_done, NULL);

    // 3. Initialize Algorithm
    // It will wake up every 100ms to sum up data from shm_ring
    algo_sum_init(&g_ctx.algo_sum, g_ctx.loop, &g_ctx.data_hub, 100);

    // 4. Initialize Protocol
    modbus_protocol_init(g_ctx.loop);

    // 5. Setup Stats/Heartbeat Timer
    uv_timer_init(g_ctx.loop, &g_ctx.stats_timer);
    g_ctx.stats_timer.data = &g_ctx;
    uv_timer_start(&g_ctx.stats_timer, on_stats_timer, 1000, 1000);

    // 6. Setup Signal Handler
    uv_signal_init(g_ctx.loop, &g_ctx.sig_term);
    uv_signal_start(&g_ctx.sig_term, on_signal, SIGINT);

    printf("[Main] Loop starting. Press Ctrl+C to exit.\n");
    uv_run(g_ctx.loop, UV_RUN_DEFAULT);

    // Cleanup
    printf("[Main] Cleaning up...\n");
    uv_loop_close(g_ctx.loop);
    shmring_close(&g_ctx.data_hub);
    // Don't unlink shm if we want persistence across restarts, but for dev we might:
    shmring_destroy(&g_ctx.data_hub); 

    return 0;
}
