#ifndef ENGINE_PRIV_INTERNAL_H
#define ENGINE_PRIV_INTERNAL_H

#include <uv.h>
#include "bus_engine.h"
#include "arbiter.h"
#include "net_buf.h"
#include "session_manager.h"

/**
 * @brief 主站引擎私有上下文
 */
typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    net_buf_pool_t *pool;
    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;

    uint8_t rx_static_data[4096];
    struct net_buf_simple rx_acc;

    int bus_id;
    uint32_t device_id; // [Nexus-Pro] 实例 ID 透传支持
    session_manager_t *session_mgr;
} master_priv_t;

#endif
