#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"

// 过压演习场景：第 2 秒正常，第 4 秒过压
static const uint8_t FRAME_NORM[] = {0x01, 0x03, 0x02, 0x00, 0xE6, 0x38, 0x4A};
static const uint8_t FRAME_OVP[] = {0x01, 0x03, 0x02, 0x01, 0x0E, 0x38, 0x4A};

typedef struct {
    uv_stream_t *stream;
    int tick;
} sim_ctx_t;

static void _on_tick(uv_timer_t *handle) {
    sim_ctx_t *ctx = handle->data;
    ctx->tick++;

    if (ctx->tick == 2) {
        LOG_INFO(LOG_CAT_CORE, "[SIM] Injecting Normal Data...");
        uv_buf_t b = uv_buf_init((char *)FRAME_NORM, sizeof(FRAME_NORM));
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_write(req, ctx->stream, &b, 1, (uv_write_cb)free);
    }
    if (ctx->tick == 4) {
        LOG_INFO(LOG_CAT_CORE, "[SIM] Injecting OVERVOLTAGE Signal!");
        uv_buf_t b = uv_buf_init((char *)FRAME_OVP, sizeof(FRAME_OVP));
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_write(req, ctx->stream, &b, 1, (uv_write_cb)free);
    }
    if (ctx->tick == 6) {
        LOG_INFO(LOG_CAT_CORE, "[SIM] Test sequence complete.");
        uv_stop(handle->loop);
    }
}

static void on_connect(uv_connect_t *req, int status) {
    sim_ctx_t *ctx = req->data;
    if (status < 0) exit(1);
    ctx->stream = req->handle;

    static uv_timer_t timer;
    uv_timer_init(req->handle->loop, &timer);
    timer.data = ctx;
    uv_timer_start(&timer, _on_tick, 1000, 1000);
}

int main(int argc, char **argv) {
    uv_loop_t *loop = uv_default_loop();
    sim_ctx_t ctx = {0};
    uv_pipe_t *uds = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, uds, 0);
    uds->data = &ctx;
    uv_connect_t *conn = malloc(sizeof(uv_connect_t));
    conn->data = &ctx;
    uv_pipe_connect(conn, uds, argc > 1 ? argv[1] : "/tmp/vbus0.sock", on_connect);
    return uv_run(loop, UV_RUN_DEFAULT);
}