#include "req_driver_legacy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "edge_driver.h"

typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    struct net_buf_pool *pool;
} master_engine_minimal_t;

typedef struct {
    driver_manager_t *dm;
    edge_driver_ctx_t *drv_ctx;
    char protocol[16];
    uint32_t slave_id;
    uint32_t tag_id;
    legacy_on_complete_cb on_complete;
    pipe_msg_t result_msg;
    void *lookup_by_tag;
    void *lookup_by_key;
} legacy_req_ctx_t;

static int legacy_setup(edge_req_t *req, void *engine_ctx) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    master_engine_minimal_t *master = (master_engine_minimal_t *)engine_ctx;
    if (!ctx->drv_ctx) {
        ctx->drv_ctx = driver_instance_create(ctx->dm, ctx->protocol, ctx->slave_id);
        if (ctx->drv_ctx) {
            ctx->drv_ctx->lookup_by_tag = (const void *(*)(uint32_t))ctx->lookup_by_tag;
            ctx->drv_ctx->lookup_by_key = (const void *(*)(uint32_t))ctx->lookup_by_key;
            ctx->drv_ctx->pool = master->pool;
        }
    }
    return ctx->drv_ctx ? 0 : -1;
}

static int legacy_encode(edge_req_t *req, net_buf_t *buf) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (!ctx->drv_ctx) return REQ_STATUS_ERROR;
    return ctx->drv_ctx->driver->encode_request(ctx->drv_ctx, &ctx->tag_id, 1, buf);
}

static int legacy_decode(edge_req_t *req, net_buf_t *buf, size_t *consumed) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (!ctx->drv_ctx) return REQ_STATUS_ERROR;
    int msg_count = 0;

    // 强制清理输出消息结构
    memset(&ctx->result_msg, 0, sizeof(pipe_msg_t));

    int rc = ctx->drv_ctx->driver->decode_response(ctx->drv_ctx, buf, NULL, &ctx->result_msg,
                                                   &msg_count, consumed);

    // [整改] 只要拿到数据，立刻宣布 DONE，绝不拖泥带水
    if (rc == 0 && msg_count > 0) {
        printf(" 📥 [IO-RX] Decoded Tag %u: Val=%.2f\n", ctx->result_msg.tag_id,
               ctx->result_msg.payload.val.f);
        fflush(stdout);
        return REQ_STATUS_DONE;
    }

    if (rc == EDGE_ERR_INCOMPLETE) return EDGE_ERR_INCOMPLETE;

    // 协议错误或其他非正常结束
    return (rc == 0) ? REQ_STATUS_DONE : REQ_STATUS_ERROR;
}

static void legacy_finish(edge_req_t *req, int status) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (ctx->on_complete) {
        ctx->on_complete(status, (status == 0) ? &ctx->result_msg : NULL);
    }
}

static void legacy_teardown(edge_req_t *req) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (ctx) {
        if (ctx->drv_ctx) driver_instance_destroy(ctx->drv_ctx, ctx->protocol);
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

edge_req_t *req_legacy_driver_create(driver_manager_t *dm, const char *protocol, uint32_t slave_id,
                                     uint32_t tag_id, void *lookup_by_tag, void *lookup_by_key,
                                     legacy_on_complete_cb user_cb) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    legacy_req_ctx_t *ctx = calloc(1, sizeof(legacy_req_ctx_t));
    ctx->dm = dm;
    ctx->slave_id = slave_id;
    ctx->tag_id = tag_id;
    ctx->lookup_by_tag = lookup_by_tag;
    ctx->lookup_by_key = lookup_by_key;
    strncpy(ctx->protocol, protocol, 15);
    ctx->on_complete = user_cb;
    req->ops = &legacy_ops;
    req->priv_data = ctx;
    return req;
}
