#include "req_raw_io.h"
#include <stdlib.h>
#include <string.h>

// Private context for the raw I/O request
typedef struct {
    net_buf_t *tx_buf; // Buffer to send
    net_buf_t *rx_buf; // Buffer for response
    raw_io_on_complete_cb on_complete;
} raw_req_ctx_t;


static int raw_encode(edge_req_t *req, net_buf_t *buf) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t*)req->priv_data;
    if (ctx->tx_buf && ctx->tx_buf->b.len > 0) {
        net_buf_append_bytes(buf, ctx->tx_buf->b.data, ctx->tx_buf->b.len);
    }
    // This is a single-shot request, so we return DONE, not CONTINUE
    return REQ_STATUS_DONE;
}

static int raw_decode(edge_req_t *req, net_buf_t *buf) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t*)req->priv_data;
    
    // For passthrough, we assume any data received completes the transaction.
    // A more complex implementation could wait for a timeout or a specific byte sequence.
    if (buf->b.len > 0) {
        // Steal the buffer from the engine's rx accumulator
        ctx->rx_buf = net_buf_clone(buf);
        net_buf_simple_reset(&buf->b); // Consume the buffer
        return REQ_STATUS_DONE;
    }
    // If no data, maybe we wait for more? For now, we assume completion.
    return REQ_STATUS_DONE;
}

static void raw_finish(edge_req_t *req, int status) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t*)req->priv_data;
    if (ctx->on_complete) {
        // Pass the captured rx_buf to the user's callback
        ctx->on_complete(status, ctx->rx_buf);
    } else if (ctx->rx_buf) {
        // If there's no callback, we must still unref the buffer
        net_buf_unref(ctx->rx_buf);
    }
}

static void raw_teardown(edge_req_t *req) {
    raw_req_ctx_t *ctx = (raw_req_ctx_t*)req->priv_data;
    if (ctx) {
        if (ctx->tx_buf) {
            net_buf_unref(ctx->tx_buf);
        }
        // rx_buf was passed to the callback, which now owns it.
        // If there was no callback, it was freed in raw_finish.
        free(ctx);
    }
    free(req);
}

static const request_ops_t raw_ops = {
    .setup = NULL, // No special setup needed
    .encode = raw_encode,
    .decode = raw_decode,
    .on_finish = raw_finish,
    .teardown = raw_teardown,
};

edge_req_t* req_raw_io_create(net_buf_t *tx_buf, raw_io_on_complete_cb user_cb) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    raw_req_ctx_t *ctx = calloc(1, sizeof(raw_req_ctx_t));

    ctx->tx_buf = tx_buf; // Assumes ownership of the reference
    ctx->rx_buf = NULL;
    ctx->on_complete = user_cb;

    req->ops = &raw_ops;
    req->priv_data = ctx;
    
    return req;
}
