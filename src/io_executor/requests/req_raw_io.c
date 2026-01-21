#include "req_raw_io.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    net_buf_t *tx_buf;
    raw_io_on_complete_cb cb;
} raw_req_ctx_t;

static int raw_encode(edge_req_t *req, net_buf_t *buf) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (!ctx->tx_buf) return REQ_STATUS_ERROR;
    net_buf_append_bytes(buf, ctx->tx_buf->b.data, ctx->tx_buf->b.len);
    return 0;
}

static int raw_decode(edge_req_t *req, net_buf_t *buf, size_t *consumed) {
    (void)req;
    *consumed = buf->b.len;
    return REQ_STATUS_DONE;
}

static void raw_finish(edge_req_t *req, int status) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (ctx->cb) ctx->cb(status, NULL);
}

static void raw_teardown(edge_req_t *req) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t *)req->priv_data;
    if (ctx) {
        if (ctx->tx_buf) net_buf_unref(ctx->tx_buf);
        free(ctx);
    }
    free(req);
}

static const request_ops_t raw_ops = {.setup = NULL,
                                      .encode = raw_encode,
                                      .decode = raw_decode,
                                      .teardown = raw_teardown,
                                      .on_finish = raw_finish};

edge_req_t *req_raw_io_create(net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    raw_req_ctx_t *ctx = calloc(1, sizeof(raw_req_ctx_t));
    ctx->tx_buf = net_buf_ref(tx_buf);
    ctx->cb = cb;
    req->ops = &raw_ops;
    req->priv_data = ctx;
    return req;
}