#include "req_raw_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    net_buf_t *tx_buf;
    net_buf_t *rx_buf;
    raw_io_on_complete_cb cb;
} raw_req_ctx_t;

static int raw_setup(edge_req_t *req, void *ctx) {
    (void)req;
    (void)ctx;
    return 0;  // 无需额外初始化
}

static int raw_encode(edge_req_t *req, net_buf_t *buf) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (!ctx->tx_buf) return REQ_STATUS_ERROR;
    net_buf_append_bytes(buf, ctx->tx_buf->b.data, ctx->tx_buf->b.len);
    return 0;
}

static int raw_decode(edge_req_t *req, net_buf_t *buf, size_t *consumed) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;

    // 捕获所有收到的数据
    if (buf->b.len > 0) {
        if (ctx->rx_buf) net_buf_unref(ctx->rx_buf);
        // 使用 clone 复制一份数据，避免底层缓冲区被回收影响上层
        ctx->rx_buf = net_buf_clone(buf, 0);
    }

    *consumed = buf->b.len;
    return REQ_STATUS_DONE;
}

static void raw_finish(edge_req_t *req, int status) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (ctx->cb) {
        ctx->cb(status, ctx->rx_buf);
    }
}

static void raw_teardown(edge_req_t *req) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (ctx) {
        if (ctx->tx_buf) net_buf_unref(ctx->tx_buf);
        if (ctx->rx_buf) net_buf_unref(ctx->rx_buf);
        free(ctx);
    }
    free(req);
}

static const request_ops_t raw_ops = {.setup = raw_setup,
                                      .encode = raw_encode,
                                      .decode = raw_decode,
                                      .teardown = raw_teardown,
                                      .on_finish = raw_finish};

edge_req_t *req_raw_io_create(net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    raw_req_ctx_t *ctx = calloc(1, sizeof(raw_req_ctx_t));
    ctx->tx_buf = net_buf_ref(tx_buf);
    ctx->rx_buf = NULL;
    ctx->cb = cb;
    req->ops = &raw_ops;
    req->priv_data = ctx;
    return req;
}
