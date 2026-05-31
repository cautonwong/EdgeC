#include "req_driver_legacy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "edge_driver.h"
#include "session_manager.h"

// [Deepening] 镜像 MasterEngine 的私有结构，用于访问 SessionManager
typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    struct net_buf_pool *pool;
    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;
    uint8_t rx_static_data[4096];
    struct net_buf_simple rx_acc;
    int bus_id;
    session_manager_t *session_mgr;
} master_engine_view_t;

typedef struct {
    driver_manager_t *dm;
    edge_driver_ctx_t *drv_ctx;
    char protocol[16];
    uint32_t slave_id;
    cmd_action_t action;
    uint32_t tag_id;
    double value;
    legacy_on_complete_cb on_complete;
    void *user_data;
    pipe_msg_t result_msg;
    void *lookup_by_tag;
    void *lookup_by_key;
} legacy_req_ctx_t;

static int legacy_setup(edge_req_t *req, void *engine_ctx) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    master_engine_view_t *master = (master_engine_view_t *)engine_ctx;

    // [Deepening] 从 SessionManager 获取持久化会话，而不是每次创建
    if (!ctx->drv_ctx) {
        ctx->drv_ctx = session_manager_get(master->session_mgr, ctx->protocol, ctx->slave_id);
        if (!ctx->drv_ctx) {
            // 如果持久化会话不存在，则自动创建一个并加入 SessionManager（主站角色）
            driver_contract_injection_t inject = {
                .meta_list = NULL,
                .meta_list_len = 0,
                .lookup_by_tag = ctx->lookup_by_tag,
                .lookup_by_key = ctx->lookup_by_key
            };
            if (session_manager_add_session(master->session_mgr, ctx->protocol, ctx->slave_id, &inject, NULL) == 0) {
                ctx->drv_ctx = session_manager_get(master->session_mgr, ctx->protocol, ctx->slave_id);
                if (ctx->drv_ctx) {
                    ctx->drv_ctx->role = PROTOCOL_ROLE_MASTER; // 设置为 Master 运行角色
                }
            }
        }
        if (ctx->drv_ctx) {
            // 注入契约查找函数 (如果是第一次创建)
            ctx->drv_ctx->lookup_by_tag = (const void *(*)(uint32_t))ctx->lookup_by_tag;
            ctx->drv_ctx->lookup_by_key = (const void *(*)(uint64_t))ctx->lookup_by_key;
            ctx->drv_ctx->pool = master->pool;
        }
    }
    return ctx->drv_ctx ? 0 : -1;
}

static int legacy_encode(edge_req_t *req, net_buf_t *buf) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver) {
        return REQ_STATUS_ERROR;
    }

    // [Deepening] 检查协议是否就绪 (例如 DLMS 是否已完成握手)
    // 如果协议有 is_ready 检查且返回 false，由驱动内部的 encode_request 处理握手逻辑
    // SessionManager 的持久化使得驱动可以记住握手状态

    if (ctx->action == CMD_ACTION_WRITE) {
        if (!ctx->drv_ctx->driver->encode_write) return REQ_STATUS_ERROR;
        return ctx->drv_ctx->driver->encode_write(ctx->drv_ctx, ctx->tag_id, ctx->value, buf);
    }

    if (!ctx->drv_ctx->driver->encode_request) return REQ_STATUS_ERROR;
    return ctx->drv_ctx->driver->encode_request(ctx->drv_ctx, &ctx->tag_id, 1, buf);
}

static int legacy_decode(edge_req_t *req, net_buf_t *buf, size_t *consumed) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver || !ctx->drv_ctx->driver->decode_response) {
        return REQ_STATUS_ERROR;
    }
    int msg_count = 0;

    memset(&ctx->result_msg, 0, sizeof(pipe_msg_t));

    int rc = ctx->drv_ctx->driver->decode_response(ctx->drv_ctx, buf, NULL, &ctx->result_msg,
                                                   &msg_count, consumed);

    if (rc == 0 && msg_count > 0) {
        if (ctx->result_msg.tag_id == ctx->tag_id) {
            return REQ_STATUS_DONE;
        } else {
            return EDGE_ERR_INCOMPLETE;
        }
    }

    if (rc == EDGE_ERR_INCOMPLETE) return EDGE_ERR_INCOMPLETE;
    if (rc != 0) return REQ_STATUS_ERROR;
    
    return (msg_count > 0 || (ctx->action == CMD_ACTION_WRITE && rc == 0)) ? REQ_STATUS_DONE
                                                                           : REQ_STATUS_ERROR;
}

static void legacy_finish(edge_req_t *req, int status) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (ctx->on_complete) {
        ctx->on_complete(status,
                         (status == 0 && ctx->action == CMD_ACTION_READ) ? &ctx->result_msg : NULL,
                         ctx->user_data);
    }
}

static void legacy_teardown(edge_req_t *req) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (ctx) {
        // [Deepening] 不再销毁 drv_ctx，它由 SessionManager 持久化管理
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
                                     cmd_action_t action, uint32_t tag_id, double value,
                                     void *lookup_by_tag, void *lookup_by_key,
                                     legacy_on_complete_cb user_cb, void *user_data) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    legacy_req_ctx_t *ctx = calloc(1, sizeof(legacy_req_ctx_t));
    ctx->dm = dm;
    ctx->slave_id = slave_id;
    ctx->action = action;
    ctx->tag_id = tag_id;
    ctx->value = value;
    ctx->lookup_by_tag = lookup_by_tag;
    ctx->lookup_by_key = lookup_by_key;
    strncpy(ctx->protocol, protocol, 15);
    ctx->on_complete = user_cb;
    ctx->user_data = user_data;
    req->ops = &legacy_ops;
    req->priv_data = ctx;
    return req;
}
