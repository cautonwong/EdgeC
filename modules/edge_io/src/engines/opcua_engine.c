#include "io_executor.h"
#include "open62541.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
    io_ops_t ops;
    uv_loop_t *loop;
    UA_Client *client;
    uv_timer_t timer;
    io_handle_t *handle;
    char endpoint[128];
    bool is_connected;
} opcua_handle_ctx_t;

static void _opcua_timer_cb(uv_timer_t *timer) {
    opcua_handle_ctx_t *ctx = (opcua_handle_ctx_t *)timer->data;
    if (ctx->is_connected && ctx->client) {
        // Run one iteration of the UA client to process async events
        UA_Client_run_iterate(ctx->client, 0);
    }
}

static int _opcua_enable_read(io_handle_t *h, bool enable) {
    opcua_handle_ctx_t *ctx = (opcua_handle_ctx_t *)h;
    if (enable) {
        if (!uv_is_active((uv_handle_t *)&ctx->timer)) {
            uv_timer_start(&ctx->timer, _opcua_timer_cb, 10, 10);
        }
    } else {
        uv_timer_stop(&ctx->timer);
    }
    return 0;
}

static int _opcua_write_async(io_handle_t *h, net_buf_t *buf) {
    (void)buf;
    // Write data to OPC UA node (Mocked for interface implementation)
    // Normally we would parse buf and issue UA_Client_writeValueAttribute
    if (h->cb) {
        h->cb(h, IO_EVENT_TX_DONE, NULL, h->user_data);
    }
    return 0;
}

static int _opcua_ioctl(io_handle_t *h, int cmd, void *arg) {
    (void)h; (void)cmd; (void)arg;
    return 0;
}

static void _opcua_on_timer_close(uv_handle_t *handle) {
    opcua_handle_ctx_t *ctx = (opcua_handle_ctx_t *)handle->data;
    if (ctx->client) {
        UA_Client_disconnect(ctx->client);
        UA_Client_delete(ctx->client);
    }
    free(ctx);
}

static void _opcua_destroy(io_handle_t *h) {
    opcua_handle_ctx_t *ctx = (opcua_handle_ctx_t *)h;
    uv_timer_stop(&ctx->timer);
    uv_close((uv_handle_t *)&ctx->timer, _opcua_on_timer_close);
}

io_handle_t *opcua_engine_create(uv_loop_t *loop, const char *endpoint) {
    opcua_handle_ctx_t *ctx = calloc(1, sizeof(opcua_handle_ctx_t));
    if (!ctx) return NULL;

    ctx->loop = loop;
    strncpy(ctx->endpoint, endpoint ? endpoint : "opc.tcp://localhost:4840", sizeof(ctx->endpoint) - 1);
    
    ctx->client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(ctx->client);
    if (cc) {
        UA_ClientConfig_setDefault(cc);
    }
    
    // Perform real connection attempt via open62541
    UA_StatusCode st = UA_Client_connect(ctx->client, ctx->endpoint);
    if (st == UA_STATUSCODE_GOOD) {
        ctx->is_connected = true;
        LOG_INFO(LOG_CAT_CORE, "OPC UA Engine: Successfully connected to %s", ctx->endpoint);
    } else {
        ctx->is_connected = false;
        LOG_WARN(LOG_CAT_CORE, "OPC UA Engine: Endpoint %s offline (StatusCode: 0x%08x), running in standby mode", ctx->endpoint, st);
    }

    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;

    ctx->ops.enable_read = _opcua_enable_read;
    ctx->ops.write_async = _opcua_write_async;
    ctx->ops.ioctl = _opcua_ioctl;
    ctx->ops.destroy = _opcua_destroy;
    
    io_handle_t *h = (io_handle_t *)ctx;
    h->ops = &ctx->ops;
    return h;
}
