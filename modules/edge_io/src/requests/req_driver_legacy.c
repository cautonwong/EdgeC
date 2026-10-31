#include "req_driver_legacy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "edge_driver.h"
#include "session_manager.h"
#include "../engines/engine_priv_internal.h"

typedef struct {
    driver_manager_t *dm;
    edge_driver_ctx_t *drv_ctx;
    char protocol[16];
    uint32_t slave_id;
    cmd_action_t action;
    uint32_t tag_id;        /* 驱动认的原生点位标识 */
    uint32_t result_tag_id; /* 回调认的标识（产品自己的 tag_id） */
    double scale;           /* 解码值量纲，<=0 视为 1.0 */
    double value;
    legacy_on_complete_cb on_complete;
    void *user_data;
    pipe_msg_t result_msg;
    void *lookup_by_tag;
    void *lookup_by_key;
    const void *meta_list;
    uint32_t meta_list_len;
    /* [ADR-002] 本次请求的点位描述（点位标识超过 32 位的协议用，如 DLMS 的 OBIS）。
     * 不是表——调用方给的就是这一个点。 */
    const void *point_desc;
    size_t point_desc_len;
    /* [ADR-002] request-native 驱动的私有请求数据（点位信息）。
     * 定长内嵌：核心路径不引入动态内存。大小由 driver->req_priv_size 决定。 */
    uint8_t drv_priv[REQ_DRV_PRIV_MAX];
    bool drv_priv_ready;
} legacy_req_ctx_t;

static int legacy_setup(edge_req_t *req, void *engine_ctx) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    master_priv_t *master = (master_priv_t *)engine_ctx;

    // [Deepening] 从 SessionManager 获取持久化会话，而不是每次创建
    if (!ctx->drv_ctx) {
        ctx->drv_ctx = session_manager_get(master->session_mgr, ctx->protocol, ctx->slave_id);
        if (!ctx->drv_ctx) {
            // 如果持久化会话不存在，则自动创建一个并加入 SessionManager（主站角色）
            driver_contract_injection_t inject = {
                .meta_list = ctx->meta_list,
                .meta_list_len = ctx->meta_list_len,
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
    if (ctx->drv_ctx) {
        /* 会话可能是上一轮建的（那时 meta_list 还是 NULL）。每次 setup 都覆盖，
         * 否则表地址永远停在广播地址 0x99×6。 */
        ctx->drv_ctx->meta_list = ctx->meta_list;
        ctx->drv_ctx->meta_list_len = ctx->meta_list_len;
        ctx->drv_ctx->device_id = ctx->slave_id;
    }

    /* [ADR-002] request-native 驱动：把点位信息填进私有数据。
     * 之后 encode/decode 都不再碰 tag_id —— 翻译由这一层完成。 */
    if (ctx->drv_ctx && ctx->drv_ctx->driver && ctx->drv_ctx->driver->req_prepare) {
        memset(ctx->drv_priv, 0, sizeof(ctx->drv_priv));
        if (ctx->drv_ctx->driver->req_prepare(ctx->drv_priv, ctx->slave_id, ctx->tag_id,
                                              ctx->meta_list, ctx->meta_list_len, ctx->point_desc,
                                              ctx->point_desc_len) != 0) {
            return -1;
        }
        ctx->drv_priv_ready = true;
    }
    return ctx->drv_ctx ? 0 : -1;
}

static int legacy_encode(edge_req_t *req, net_buf_t *buf) {
    legacy_req_ctx_t *ctx = (legacy_req_ctx_t *)req->priv_data;
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver) {
        return REQ_STATUS_ERROR;
    }

    if (ctx->drv_ctx->driver->req_encode) {
        if (!ctx->drv_priv_ready) return REQ_STATUS_ERROR;
        /* session = 设备级持久上下文（ctx_size 字节），跨请求保留 APCI 序号等会话状态 */
        void *session = ctx->drv_ctx->priv;
        return ctx->drv_ctx->driver->req_encode(ctx->drv_priv, session, buf) == 0
                   ? 0
                   : REQ_STATUS_ERROR;
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
    if (!ctx->drv_ctx || !ctx->drv_ctx->driver) return REQ_STATUS_ERROR;

    /* [ADR-002] request-native：驱动只回值，tag_id 由**请求层**贴上。
     * 这正是 L2 的修法——驱动不再把 DI 当 tag_id 抛出来。 */
    if (ctx->drv_ctx->driver->req_decode) {
        if (!ctx->drv_priv_ready) return REQ_STATUS_ERROR;
        double value = 0.0;
        int nrc = ctx->drv_ctx->driver->req_decode(ctx->drv_priv, ctx->drv_ctx->priv,
                                                   buf->b.data, buf->b.len, consumed, &value);
        if (nrc == EDGE_ERR_INCOMPLETE) return EDGE_ERR_INCOMPLETE;
        if (nrc != 0) return REQ_STATUS_ERROR;

        memset(&ctx->result_msg, 0, sizeof(pipe_msg_t));
        /* 驱动只回原始值；标识与量纲由调用方给（ADR-002：驱动不认 tag_id、不管标定） */
        pipe_msg_init_scalar(&ctx->result_msg, (edge_tag_id_t)ctx->result_tag_id,
                             value * ctx->scale, uv_hrtime());
        ctx->result_msg.device_id = ctx->slave_id;
        return REQ_STATUS_DONE;
    }

    if (!ctx->drv_ctx->driver->decode_response) {
        return REQ_STATUS_ERROR;
    }
    int msg_count = 0;

    memset(&ctx->result_msg, 0, sizeof(pipe_msg_t));

    ctx->drv_ctx->session_state = (int)ctx->tag_id;

    int rc = ctx->drv_ctx->driver->decode_response(ctx->drv_ctx, buf, NULL, &ctx->result_msg,
                                                   &msg_count, consumed);

    if (rc == 0 && msg_count > 0) {
        /* shadow 按 (tag, device_id) 索引；驱动 decode 不填 device_id 的话
         * datahub_read_shadow_device(tag, meter_idx) 只有 index 0 命得中。 */
        ctx->result_msg.device_id = ctx->slave_id;
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
                                     cmd_action_t action, uint32_t native_id, uint32_t result_tag_id,
                                     double scale, double value, void *lookup_by_tag,
                                     void *lookup_by_key, const void *meta_list,
                                     uint32_t meta_list_len, const void *point_desc,
                                     size_t point_desc_len, legacy_on_complete_cb user_cb,
                                     void *user_data) {
    edge_req_t *req = calloc(1, sizeof(edge_req_t));
    legacy_req_ctx_t *ctx = calloc(1, sizeof(legacy_req_ctx_t));
    ctx->dm = dm;
    ctx->slave_id = slave_id;
    ctx->action = action;
    ctx->tag_id = native_id;
    ctx->result_tag_id = result_tag_id;
    ctx->scale = (scale > 0.0) ? scale : 1.0;
    ctx->value = value;
    /* [ADR-002] request-native 驱动（req_encode 非空）根本不会碰这两个指针；
     * 仍在 legacy 的驱动（Modbus/IEC104/…）还要靠它们查表，所以照旧传下去，
     * 等它们迁完再整块删掉（届时这两个形参一起消失）。 */
    ctx->lookup_by_tag = lookup_by_tag;
    ctx->lookup_by_key = lookup_by_key;
    ctx->meta_list = meta_list;
    ctx->point_desc = point_desc;
    ctx->point_desc_len = point_desc_len;
    ctx->meta_list_len = meta_list_len;
    strncpy(ctx->protocol, protocol, 15);
    ctx->on_complete = user_cb;
    ctx->user_data = user_data;
    req->ops = &legacy_ops;
    req->priv_data = ctx;
    return req;
}
