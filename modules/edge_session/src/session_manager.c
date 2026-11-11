#include "session_manager.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "edge_log.h"

typedef struct {
    edge_driver_ctx_t *ctx;
    char protocol[16];
    uint32_t device_id;
    bool in_use;
} session_entry_t;

struct session_manager_s {
    edge_driver_session_port_t driver_port;
    session_entry_t *sessions;
    int max_sessions;
    int current_count;

    /* [ADR-002 S3-0] 从站面端口：由组合根注入，供 request-native 驱动的
     * req_slave_exchange 取数/收数。端口不进 edge_driver_ctx_t —— 进了 ctx，
     * 驱动就又变成"自己取数"，等于把 read_shadow 换个名字请回来。 */
    edge_slave_source_ops_t slave_src;
    void *slave_src_ctx;
    /* [ADR-002] 北向生产者端口：与从站端口并列，用途不同（BATCH/EVENT/标量） */
    edge_north_source_ops_t north_src;
    void *north_src_ctx;
    /* 没注入过就传 NULL，而不是传一个全零的结构体——后者会让驱动里
     * `if (!src) return -1;` 的守卫失效，一路走到 `src->peek(...)` 才崩。 */
    bool north_src_set;
    /* 驱动会把这块 scratch 强转成自己的请求结构体（里面有 double / 指针），
     * 所以它必须满足 max_align_t。**少了 alignas 就是 UB**：紧跟在上面的
     * `bool north_src_set` 只有 1 字节对齐，数组起始落在 8 字节边界外一个字节上，
     * ubsan 直接报 "member access within misaligned address"，普通构建只是"碰巧能跑"。
     * `driver_manager.c` 的 priv 早就加了 alignas，这里当时漏了。 */
    alignas(max_align_t) uint8_t slave_scratch[REQ_DRV_PRIV_MAX];
};

void session_manager_set_north_source(session_manager_t *sm, const edge_north_source_ops_t *ops,
                                      void *ctx) {
    if (!sm) return;
    if (ops) {
        sm->north_src = *ops;
        sm->north_src_set = true;
    } else {
        memset(&sm->north_src, 0, sizeof(sm->north_src));
        sm->north_src_set = false;
    }
    sm->north_src_ctx = ctx;
}

void session_manager_set_slave_source(session_manager_t *sm, const edge_slave_source_ops_t *ops,
                                      void *ctx) {
    if (!sm) return;
    if (ops) {
        sm->slave_src = *ops;
    } else {
        memset(&sm->slave_src, 0, sizeof(sm->slave_src));
    }
    sm->slave_src_ctx = ctx;
}

session_manager_t *session_manager_create(const edge_driver_session_port_t *port, int max_sessions) {
    session_manager_t *sm = calloc(1, sizeof(session_manager_t));
    if (port) sm->driver_port = *port;
    sm->max_sessions = max_sessions;
    sm->sessions = calloc((size_t)max_sessions, sizeof(session_entry_t));
    return sm;
}

void session_manager_destroy(session_manager_t *sm) {
    if (!sm) return;
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use) {
            if (sm->driver_port.destroy) sm->driver_port.destroy(sm->sessions[i].ctx);
        }
    }
    free(sm->sessions);
    free(sm);
}

edge_driver_ctx_t *session_manager_get(session_manager_t *sm, const char *protocol, uint32_t device_id) {
    if (!sm || !protocol) return NULL;
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use && 
            sm->sessions[i].device_id == device_id &&
            strcmp(sm->sessions[i].protocol, protocol) == 0) {
            return sm->sessions[i].ctx;
        }
    }
    return NULL;
}

int session_manager_add_session(session_manager_t *sm, const char *protocol, uint32_t device_id,
                                void *user_data) {
    if (!sm || !protocol) return -1;
    for (int i = 0; i < sm->max_sessions; i++) {
        if (!sm->sessions[i].in_use) {
            edge_driver_ctx_t *ctx = sm->driver_port.create
                                          ? sm->driver_port.create(sm->driver_port.ctx, protocol,
                                                                   device_id)
                                          : NULL;
            if (!ctx) return -1;
            
            ctx->user_data = user_data;
            ctx->role = PROTOCOL_ROLE_SLAVE;
            
            sm->sessions[i].ctx = ctx;
            strncpy(sm->sessions[i].protocol, protocol, 15);
            sm->sessions[i].device_id = device_id;
            sm->sessions[i].in_use = true;
            sm->current_count++;
            LOG_INFO(LOG_CAT_IO, "Session Created: %s:%u (Total: %d)", protocol, device_id, sm->current_count);
            return 0;
        }
    }
    return -1;
}

int session_manager_handle_rx(session_manager_t *sm, net_buf_t *rx, net_buf_t *resp_out,
                              pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    if (!sm) return -1;
    *async_count = 0;
    *consumed = 0;
    
    // Simple dispatcher: try all active sessions
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use) {
            edge_driver_ctx_t *ctx = sm->sessions[i].ctx;
            if (!ctx->driver) continue;

            /* request-native：驱动只做编解码，取值/收值走注入的端口 */
            if (ctx->driver->req_slave_exchange) {
                memset(sm->slave_scratch, 0, sizeof(sm->slave_scratch));
                int rc = ctx->driver->req_slave_exchange(sm->slave_scratch, ctx->priv, rx->b.data,
                                                         rx->b.len, &sm->slave_src,
                                                         sm->slave_src_ctx, resp_out, consumed);
                if (rc != EDGE_ERR_INCOMPLETE) return rc;
                continue;
            }

            if (ctx->driver->handle_rx) {
                int rc = ctx->driver->handle_rx(ctx, rx, resp_out, async_msgs, async_count, consumed);
                if (rc != EDGE_ERR_INCOMPLETE) return rc;
            }
        }
    }
    return EDGE_ERR_INCOMPLETE;
}

int session_manager_tick(session_manager_t *sm, int bus_id, net_buf_t **out_txs, int max_txs) {
    (void)bus_id;
    if (!sm) return 0;
    int tx_count = 0;
    for (int i = 0; i < sm->max_sessions && tx_count < max_txs; i++) {
        if (sm->sessions[i].in_use) {
            edge_driver_ctx_t *ctx = sm->sessions[i].ctx;
            if (!ctx->driver) continue;

            /* request-native 生产者：状态由端口给，驱动不碰 datahub */
            if (ctx->driver->req_tick) {
                net_buf_t *tx = NULL;
                if (ctx->driver->req_tick(ctx->priv, sm->north_src_set ? &sm->north_src : NULL,
                                          sm->north_src_ctx, &sm->slave_src,
                                          sm->slave_src_ctx, ctx->pool, &tx) == 0 &&
                    tx) {
                    out_txs[tx_count++] = tx;
                }
                continue;
            }

            if (ctx->driver->on_tick) {
                net_buf_t *tx = NULL;
                if (ctx->driver->on_tick(ctx, &tx) == 0 && tx) {
                    out_txs[tx_count++] = tx;
                }
            }
        }
    }
    return tx_count;
}

void session_manager_close(session_manager_t *sm, const char *protocol, uint32_t device_id) {
    if (!sm || !protocol) return;
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use && 
            sm->sessions[i].device_id == device_id &&
            strcmp(sm->sessions[i].protocol, protocol) == 0) {
            if (sm->driver_port.destroy) sm->driver_port.destroy(sm->sessions[i].ctx);
            sm->sessions[i].in_use = false;
            sm->current_count--;
            LOG_INFO(LOG_CAT_IO, "Session Closed: %s:%u", protocol, device_id);
            return;
        }
    }
}
