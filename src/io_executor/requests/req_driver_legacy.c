#include "req_driver_legacy.h"
#include "arbiter.h"
#include "edge_driver.h"
#include <stdlib.h>
#include <string.h>

// Private context for the legacy request
typedef struct {
    driver_manager_t *dm;
    edge_driver_ctx_t *drv_ctx;
    char protocol[16];
    uint32_t tag_id;
    // We store the original callback here
    legacy_on_complete_cb on_complete;
} legacy_req_ctx_t;

static int legacy_setup(edge_req_t *req, void *engine_ctx) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t*)req->priv_data;
    // The master_engine's private struct is passed as engine_ctx.
    // We can extract the memory pool from it if needed in the future.
    
    // This is where the logic from the old master_engine lives now
    ctx->drv_ctx = driver_instance_create(ctx->dm, ctx->protocol, ctx->tag_id);
    if (!ctx->drv_ctx) {
        return -1;
    }
    ctx->drv_ctx->role = PROTOCOL_ROLE_MASTER;
    // ctx->drv_ctx->pool = ((master_priv_t*)engine_ctx)->pool; // Example of getting the pool
    return 0;
}

static int legacy_encode(edge_req_t *req, net_buf_t *buf) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t*)req->priv_data;
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver->encode_request) {
        return REQ_STATUS_ERROR;
    }
    // Call the original driver's encode function
    return ctx->drv_ctx->driver->encode_request(ctx->drv_ctx, &ctx->tag_id, 1, buf);
}

static int legacy_decode(edge_req_t *req, net_buf_t *buf) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t*)req->priv_data;
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver->decode_response) {
        return REQ_STATUS_ERROR;
    }

    // The decode function now needs a place to store its results.
    // We'll attach them to the private context.
    pipe_msg_t result_msg;
    int msg_count = 0;
    size_t consumed = 0;
    net_buf_t *resp_out = net_buf_alloc(NULL, 0); // Not used in this legacy path

    int rc = ctx->drv_ctx->driver->decode_response(ctx->drv_ctx, buf, resp_out, &result_msg, &msg_count, &consumed);
    
    net_buf_unref(resp_out);
    
    if (rc == 0 && msg_count > 0) {
        // Success. Store result for on_finish.
        // For simplicity, we assume the first message has the value.
        // A more robust implementation would handle multiple messages.
        req->priv_data = realloc(req->priv_data, sizeof(legacy_req_ctx_t) + sizeof(pipe_msg_t));
        ctx = req->priv_data;
        memcpy(ctx + 1, &result_msg, sizeof(pipe_msg_t));
        return REQ_STATUS_DONE;
    } else if (rc == EDGE_ERR_INCOMPLETE) {
        return REQ_STATUS_CONTINUE; // Tell engine to wait for more data
    }

    return rc == 0 ? REQ_STATUS_DONE : REQ_STATUS_ERROR;
}

static void legacy_finish(edge_req_t *req, int status) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t*)req->priv_data;
    if (ctx->on_complete) {
        pipe_msg_t *result_msg = NULL;
        if (status == CMD_STATUS_SUCCESS) {
            // Retrieve the stored result from decode
            result_msg = (pipe_msg_t*)(ctx + 1);
        }
        // Call the original user callback
        ctx->on_complete(status, result_msg);
    }
}

static void legacy_teardown(edge_req_t *req) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t*)req->priv_data;
    if (ctx) {
        if (ctx->drv_ctx) {
            driver_instance_destroy(ctx->drv_ctx, ctx->protocol);
        }
        free(ctx);
    }
    free(req);
}

static const request_ops_t legacy_ops = {
    .setup = legacy_setup,
    .encode = legacy_encode,
    .decode = legacy_decode,
    .on_finish = legacy_finish,
    .teardown = legacy_teardown,
};

edge_req_t* req_legacy_driver_create(driver_manager_t *dm, const char *protocol, uint32_t tag_id, legacy_on_complete_cb user_cb) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    legacy_req_ctx_t *ctx = calloc(1, sizeof(legacy_req_ctx_t));

    ctx->dm = dm;
    ctx->tag_id = tag_id;
    strncpy(ctx->protocol, protocol, 15);
    ctx->on_complete = user_cb;

    req->ops = &legacy_ops;
    req->priv_data = ctx;
    
    return req;
}
