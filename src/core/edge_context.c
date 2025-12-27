#include "core/edge_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern edge_driver_t modbus_rtu_driver;

static void on_poll_timer(uv_timer_t *handle) {
    edge_ctx_t *ctx = (edge_ctx_t *)handle->data;
    if (ctx->shutting_down) return;

    for (size_t i = 0; i < ctx->config->point_count; i++) {
        edge_request_read(ctx->arbiter, (arbiter_bus_t)ctx->config->bus_id, &modbus_rtu_driver,
                          &ctx->config->points[i], ctx->config->slave_id, &ctx->data_hub.ring);
    }
}

static void on_signal(uv_signal_t *handle, int signum) {
    (void)signum;
    edge_ctx_t *ctx = (edge_ctx_t *)handle->data;
    edge_ctx_stop(ctx);
}

static void on_hub_data(datahub_t *hub, void *user_data) {
    (void)hub;
    (void)user_data;
}

int edge_ctx_init(edge_ctx_t *ctx, const char *config_file) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(edge_ctx_t));

    ctx->config = profile_load(config_file);
    if (!ctx->config) return -1;

    ctx->loop = uv_default_loop();

    // 1. Initialize Memory Pools (REPLACES OLD mem_sys_init)
    // 256 packets of 256 bytes (Typical Modbus/CAN frames)
    if (net_buf_pool_init(&ctx->packet_pool, 256, 256, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to init packet pool\n");
        return -1;
    }

    shm_unlink("/edgeos_hub");
    int r = datahub_init(&ctx->data_hub, ctx->loop, "/edgeos_hub", 128, on_hub_data, ctx);
    if (r != 0) return r;

    io_executor_init(ctx->loop);
    ctx->arbiter = arbiter_create(ctx->loop);

    // Pass pools to services if needed, or services can access ctx via user_data if redesigned
    // For now, services allocate their own buffers or use stack,
    // but future IO jobs should use &ctx->packet_pool.

    algo_sum_init(&ctx->algo_sum, ctx->loop, &ctx->data_hub.ring, 100);
    persistence_mgr_init(&ctx->storage, ctx->loop, &ctx->data_hub.ring, 5000);
    northbound_mgr_init(&ctx->cloud, ctx->loop, &ctx->data_hub.ring);
    sys_supervisor_init(&ctx->supervisor, ctx->loop, &ctx->data_hub.ring);

    uv_timer_init(ctx->loop, &ctx->poll_timer);
    ctx->poll_timer.data = ctx;
    uv_timer_start(&ctx->poll_timer, on_poll_timer, 1000, 1000);

    uv_signal_init(ctx->loop, &ctx->sig_int);
    ctx->sig_int.data = ctx;
    uv_signal_start(&ctx->sig_int, on_signal, SIGINT);

    uv_signal_init(ctx->loop, &ctx->sig_term);
    ctx->sig_term.data = ctx;
    uv_signal_start(&ctx->sig_term, on_signal, SIGTERM);

    ctx->shutting_down = 0;
    return 0;
}

int edge_ctx_start(edge_ctx_t *ctx) {
    if (!ctx) return -1;
    printf("EdgeOS Industrial Core running...\n");
    return uv_run(ctx->loop, UV_RUN_DEFAULT);
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

void edge_ctx_stop(edge_ctx_t *ctx) {
    if (!ctx || ctx->shutting_down) return;
    printf("\nEdgeOS shutting down gracefully...\n");
    ctx->shutting_down = 1;

    io_executor_shutdown();
    arbiter_destroy(ctx->arbiter);
    algo_sum_close(&ctx->algo_sum);
    persistence_mgr_close(&ctx->storage);
    sys_supervisor_close(&ctx->supervisor);
    datahub_destroy(&ctx->data_hub);

    profile_free(ctx->config);

    // Cleanup Pools
    net_buf_pool_destroy(&ctx->packet_pool);

    uv_timer_stop(&ctx->poll_timer);
    uv_signal_stop(&ctx->sig_int);
    uv_signal_stop(&ctx->sig_term);

    uv_walk(ctx->loop, on_walk_close, NULL);

    uv_run(ctx->loop, UV_RUN_DEFAULT);
}