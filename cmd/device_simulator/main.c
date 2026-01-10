/**
 * @file main.c
 * @brief Device Simulator - The Black-Box Test Orchestrator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "net_buf.h"

// 简单的剧本命令：
// PUSH 1001 275.0  -> 发送 Tag 1001 值为 275 的规约报文 (此处简化为直接二进制或伪报文)
// EXPECT 8001 1.0  -> 期待收到 Tag 8001 值为 1 的指令

typedef struct {
    uv_stream_t *stream;
    edge_log_t *logger;
    uv_timer_t scenario_timer;
    int step;
} sim_ctx_t;

// 模拟 Modbus RTU 响应报文 (Tag 1001 -> 电压 275V)
static const uint8_t MOCK_OVP_FRAME[] = {0x01, 0x03, 0x02, 0x01, 0x0E, 0x38, 0x4A};

static void on_write_done(uv_write_t *req, int status) {
    free(req);
    (void)status;
}

static void run_scenario_step(uv_timer_t *handle) {
    sim_ctx_t *ctx = handle->data;
    ctx->step++;

    if (ctx->step == 2) {
        LOG_WARN(LOG_CAT_CORE, "[SIMULATOR] Step 2: Injecting Overvoltage (275V) Modbus Frame...");
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_buf_t buf = uv_buf_init((char *)MOCK_OVP_FRAME, sizeof(MOCK_OVP_FRAME));
        uv_write(req, ctx->stream, &buf, 1, on_write_done);
    }

    if (ctx->step == 5) {
        LOG_INFO(LOG_CAT_CORE, "[SIMULATOR] Scenario complete. Exiting...");
        uv_stop(handle->loop);
    }
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    sim_ctx_t *ctx = stream->data;
    if (nread > 0) {
        LOG_INFO(LOG_CAT_CORE,
                 "[SIMULATOR] Received %ld bytes from IEMS. Analyzing control command...", nread);
        // 此处可解析是否为预期的 REFLEX 指令
    }
    if (buf->base) free(buf->base);
}

static void on_alloc(uv_handle_t *handle, size_t suggested, uv_buf_t *buf) {
    (void)handle;
    buf->base = malloc(suggested);
    buf->len = (unsigned int)suggested;
}

static void on_connect(uv_connect_t *req, int status) {
    sim_ctx_t *ctx = req->data;
    if (status < 0) {
        LOG_ERROR(LOG_CAT_CORE, "[SIMULATOR] Connection failed: %s", uv_strerror(status));
        return;
    }
    LOG_INFO(LOG_CAT_CORE, "[SIMULATOR] Connected to IEMS vBus. Starting scenario...");
    ctx->stream = req->handle;
    uv_read_start(ctx->stream, on_alloc, on_read);

    uv_timer_start(&ctx->scenario_timer, run_scenario_step, 1000, 1000);
}

static void on_close_free(uv_handle_t *handle) {

    free(handle);

}



int main(int argc, char **argv) {

    const char *vbus_path = "/tmp/vbus0.sock";

    if (argc > 1) vbus_path = argv[1];



    uv_loop_t *loop = uv_default_loop();

    edge_log_t *logger = edge_log_create(NULL, true);



    sim_ctx_t ctx = { .logger = logger, .step = 0 };

    uv_timer_init(loop, &ctx.scenario_timer);

    ctx.scenario_timer.data = &ctx;



    uv_pipe_t *uds = malloc(sizeof(uv_pipe_t));

    uv_pipe_init(loop, uds, 0);

    uds->data = &ctx;



    uv_connect_t *conn = malloc(sizeof(uv_connect_t));

    conn->data = &ctx;

    

    LOG_INFO(LOG_CAT_CORE, "[SIMULATOR] Attempting to connect to IEMS at %s", vbus_path);

    uv_pipe_connect(conn, uds, vbus_path, on_connect);



    uv_run(loop, UV_RUN_DEFAULT);



    // 清理

    uv_close((uv_handle_t *)&ctx.scenario_timer, NULL);

    uv_close((uv_handle_t *)uds, on_close_free);

    free(conn);

    

    // 彻底排空 loop

    while (uv_run(loop, UV_RUN_NOWAIT));

    uv_loop_close(loop);



    edge_log_destroy(logger);

    return 0;

}


