#include "core/edge_context.h"
#include "io_executor.h"
#include <stdio.h>

// A dummy callback for the datahub for now
static void on_hub_data(datahub_t *hub, void *user_data) {
    (void)hub;
    (void)user_data;
    // In a real app, this would trigger application logic
    printf("Data received on hub.\n");
}

int edge_ctx_init(edge_ctx_t *ctx) {
    if (!ctx) return -1;

    int r = 0;

    // Initialize the main event loop
    r = uv_loop_init(&ctx->loop);
    if (r != 0) return r;

    // Initialize core modules
    // These modules need the main loop to post events back to.
    r = io_executor_init(&ctx->loop);
    if (r != 0) return r;
    
    ctx->arbiter = arbiter_create(&ctx->loop);
    if (!ctx->arbiter) return -1;
    
    // The datahub is the central data bus
    r = datahub_init(&ctx->data_hub, &ctx->loop, "/edge_data_hub", 256, on_hub_data, ctx);
    if (r != 0) {
        arbiter_destroy(ctx->arbiter);
        return r;
    }

    ctx->shutting_down = 0;

    printf("EdgeContext initialized successfully.\n");
    return 0;
}

int edge_ctx_start(edge_ctx_t *ctx) {
    if (!ctx) return -1;

    printf("EdgeContext starting event loop...\n");
    return uv_run(&ctx->loop, UV_RUN_DEFAULT);
}

void edge_ctx_stop(edge_ctx_t *ctx) {
    if (!ctx || ctx->shutting_down) return;

    printf("EdgeContext shutting down...\n");
    ctx->shutting_down = 1;

    // 1. Shut down modules that post events to the loop
    io_executor_shutdown();
    arbiter_destroy(ctx->arbiter);
    datahub_destroy(&ctx->data_hub);

    // 2. Stop the event loop
    uv_stop(&ctx->loop);

    // 3. Ensure all close callbacks are processed
    // This is important to let libuv clean up handles
    uv_run(&ctx->loop, UV_RUN_NOWAIT);

    // 4. Close the loop itself
    int r = uv_loop_close(&ctx->loop);
    if (r == UV_EBUSY) {
        // This can happen if some handles were not closed properly.
        // Forcibly run one more time.
        uv_run(&ctx->loop, UV_RUN_ONCE);
        uv_loop_close(&ctx->loop);
    }
    printf("EdgeContext shut down complete.\n");
}
