#include "session_manager.h"
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
    driver_manager_t *driver_mgr;
    session_entry_t *sessions;
    int max_sessions;
    int current_count;
};

session_manager_t *session_manager_create(driver_manager_t *mgr, int max_sessions) {
    session_manager_t *sm = calloc(1, sizeof(session_manager_t));
    sm->driver_mgr = mgr;
    sm->max_sessions = max_sessions;
    sm->sessions = calloc(max_sessions, sizeof(session_entry_t));
    return sm;
}

void session_manager_destroy(session_manager_t *sm) {
    if (!sm) return;
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use) {
            driver_instance_destroy(sm->sessions[i].ctx, sm->sessions[i].protocol);
        }
    }
    free(sm->sessions);
    free(sm);
}

edge_driver_ctx_t *session_manager_get(session_manager_t *sm, const char *protocol, uint32_t device_id) {
    if (!sm || !protocol) return NULL;

    // 1. 查找现有会话
    for (int i = 0; i < sm->max_sessions; i++) {
        if (sm->sessions[i].in_use && 
            sm->sessions[i].device_id == device_id &&
            strcmp(sm->sessions[i].protocol, protocol) == 0) {
            return sm->sessions[i].ctx;
        }
    }

    // 2. 创建新会话
    for (int i = 0; i < sm->max_sessions; i++) {
        if (!sm->sessions[i].in_use) {
            edge_driver_ctx_t *ctx = driver_instance_create(sm->driver_mgr, protocol, device_id);
            if (!ctx) return NULL;

            sm->sessions[i].ctx = ctx;
            strncpy(sm->sessions[i].protocol, protocol, 15);
            sm->sessions[i].device_id = device_id;
            sm->sessions[i].in_use = true;
            sm->current_count++;
            
            LOG_INFO(LOG_CAT_IO, "Session Created: %s:%u (Total: %d)", protocol, device_id, sm->current_count);
            return ctx;
        }
    }

    LOG_ERROR(LOG_CAT_IO, "Session Manager Pool Exhausted (%d)", sm->max_sessions);
    return NULL;
}

int session_manager_tick(session_manager_t *sm, int bus_id, net_buf_t **out_txs, int max_txs) {
    if (!sm) return 0;
    int tx_count = 0;
    for (int i = 0; i < sm->max_sessions && tx_count < max_txs; i++) {
        if (sm->sessions[i].in_use) {
            edge_driver_ctx_t *ctx = sm->sessions[i].ctx;
            if (ctx->driver && ctx->driver->on_tick) {
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
            
            driver_instance_destroy(sm->sessions[i].ctx, sm->sessions[i].protocol);
            sm->sessions[i].in_use = false;
            sm->current_count--;
            LOG_INFO(LOG_CAT_IO, "Session Closed: %s:%u", protocol, device_id);
            return;
        }
    }
}
