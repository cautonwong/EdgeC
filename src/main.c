#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <uv.h>

#include "edge_version.h"
#include "shm_ring.h"
#include "arbiter.h"
#include "io_executor.h"
#include "algorithm_sum.h"
#include "sqlite_service.h"
#include "modbus_protocol.h"
#include "driver_manager.h" // Include the new driver manager



// Application context holds all major components
typedef struct {
    uv_loop_t *loop;
    arbiter_t *arbiter;
    shm_ring_t data_hub;
    driver_manager_t *driver_manager; // Use driver manager
    
    // Components
    algo_sum_t algo_sum;
    
    // Timers
    uv_timer_t poll_timer;
    uv_timer_t persist_timer;

    // Signal handler for graceful shutdown
    uv_signal_t sigint_handle;
} app_ctx_t;

// Forward declarations of callbacks
static void on_io_done(io_job_t *job, void *user);

// --- Modbus Protocol -> DataHub Callback ---
static void on_modbus_read(int status, const uint16_t *registers, uint16_t num_registers, void *user) {
    app_ctx_t *ctx = (app_ctx_t *)user;
    if (status == 0) {
        printf("[Main] Modbus read success. Register[0] = %u\n", registers[0]);
        // Publish to DataHub
        PipeMsg msg = {
            .type = PIPE_SCALAR_F64,
            .payload.val_f = (double)registers[0]
        };
        shm_ring_write(&ctx->data_hub, &msg);
    } else {
        printf("[Main] Modbus read failed with status: %d\n", status);
    }
}

// --- IO Executor Callback ---
static void on_io_done(io_job_t *job, void *user) {
    printf("[Main] IO job finished.\n");
    app_ctx_t *ctx = (app_ctx_t *)user;
    
    modbus_protocol_handle_response(job);
    
    printf("[Main] Releasing bus...\n");
    arbiter_release(ctx->arbiter, job->bus, ctx);
    
    if (job) { free(job->tx); free(job->rx); free(job); }
}

// --- Arbiter Callback ---
static void on_bus_granted(arbiter_t *arb, arbiter_bus_t bus, void *user) {
    printf("[Main] Bus %d granted.\n", bus);
    app_ctx_t *ctx = (app_ctx_t *)user;

    int fd = driver_manager_get_fd(ctx->driver_manager, bus);
    if (fd < 0) {
        fprintf(stderr, "Failed to get FD for bus %d.\n", bus);
        arbiter_release(arb, bus, user);
        return;
    }

    io_job_t *job = modbus_protocol_create_read_job(1, 100, 2, 256, on_modbus_read, ctx);
    if (!job) {
        fprintf(stderr, "Failed to create modbus job.\n");
        arbiter_release(arb, bus, user);
        return;
    }
    job->bus = bus;
    job->fd = fd;

    printf("[Main] Submitting IO job...\n");
    io_submit(ctx->loop, job, on_io_done, ctx);
}

// --- Top-level Business Logic ---
static void poll_task(uv_timer_t *timer) {
    app_ctx_t *ctx = (app_ctx_t *)timer->data;
    printf("\n[Main] Polling timer triggered. Requesting bus...\n");
    arbiter_request(ctx->arbiter, BUS_RS485_1, ARB_PRIO_NORMAL, ARB_SHARED, 5000, on_bus_granted, ctx);
}

// --- Persistence Logic ---
static void persist_task(uv_timer_t *timer) {
    app_ctx_t *ctx = (app_ctx_t *)timer->data;
    PipeMsg msg;
    uint64_t seq = 0;
    
    while (shm_ring_read(&ctx->data_hub, &seq, &msg) == 0) {
        char sql[128];
        snprintf(sql, sizeof(sql), "INSERT INTO readings (value) VALUES (%.2f);", msg.payload.val_f);
        printf("[Main] Persisting value %.2f to DB...\n", msg.payload.val_f);
        sqlite_exec_async(sql, NULL, NULL);
    }
}

// --- Graceful Shutdown ---
void on_sigint(uv_signal_t *handle, int signum) {
    printf("\n[Main] SIGINT received. Shutting down gracefully...\n");
    app_ctx_t *ctx = (app_ctx_t *)handle->data;
    
    uv_timer_stop(&ctx->poll_timer);
    uv_timer_stop(&ctx->persist_timer);
    uv_timer_stop(&ctx->algo_sum.timer);
    
    arbiter_destroy(ctx->arbiter);
    driver_manager_destroy(ctx->driver_manager); // Destroy driver manager
    shm_ring_destroy(&ctx->data_hub);
    sqlite_service_close();
    
    uv_signal_stop(handle);
}

int main(void) {
    printf("Starting %s...\n", edge_get_version());

    app_ctx_t ctx = {0};
    ctx.loop = uv_default_loop();

    // --- Component Initialization ---
    ctx.driver_manager = driver_manager_create();
    if (!ctx.driver_manager) return 1;
    if (shm_ring_init(&ctx.data_hub, 128) != 0) return 1;
    ctx.arbiter = arbiter_create(ctx.loop);
    if (!ctx.arbiter) return 1;
    if (sqlite_service_init(ctx.loop, "edge_data.db") != 0) return 1;
    if (algo_sum_init(&ctx.algo_sum, ctx.loop, &ctx.data_hub, 2000) != 0) return 1;

    sqlite_exec_async("CREATE TABLE IF NOT EXISTS readings(id INTEGER PRIMARY KEY, value REAL, ts DATETIME DEFAULT CURRENT_TIMESTAMP);", NULL, NULL);

    // --- Timers for Business Logic ---
    uv_timer_init(ctx.loop, &ctx.poll_timer);
    ctx.poll_timer.data = &ctx;
    uv_timer_start(&ctx.poll_timer, poll_task, 1000, 5000);

    uv_timer_init(ctx.loop, &ctx.persist_timer);
    ctx.persist_timer.data = &ctx;
    uv_timer_start(&ctx.persist_timer, persist_task, 3000, 10000);

    // --- Signal Handling for Shutdown ---
    uv_signal_init(ctx.loop, &ctx.sigint_handle);
    ctx.sigint_handle.data = &ctx;
    uv_signal_start(&ctx.sigint_handle, on_sigint, SIGINT);

    printf("[Main] Application initialized. Running... (Press Ctrl+C to exit)\n");
    uv_run(ctx.loop, UV_RUN_DEFAULT);
    
    // --- Cleanup after loop exits ---
    // The on_sigint handler now takes care of destroying components.
    // We just need to make sure all handles are closed before closing the loop.
    uv_close((uv_handle_t*)&ctx.poll_timer, NULL);
    uv_close((uv_handle_t*)&ctx.persist_timer, NULL);
    uv_close((uv_handle_t*)&ctx.algo_sum.timer, NULL);
    uv_close((uv_handle_t*)&ctx.sigint_handle, NULL);
    
    uv_run(ctx.loop, UV_RUN_ONCE);
    uv_loop_close(ctx.loop);

    printf("[Main] Shutdown complete.\n");
    return 0;
}