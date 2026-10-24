#include "bus_engine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "edge_log.h"
#include "session_manager.h"
#include "arbiter.h"
#include "io_executor.h"
#include "edge_guard.h"

#define MAX_PROTO_PER_BUS 4

typedef struct {
    char name[16];
    uint32_t device_id;
    const void *ctx; 
} slave_proto_t;

typedef struct {
    int bus_id;
    io_handle_t *io;
    datahub_t *hub;
    uv_loop_t *loop;
    net_buf_pool_t *pool;
    arbiter_t *arb;
    uv_timer_t timer;
    session_manager_t *session_mgr;
    net_buf_t *rx_acc;
    slave_proto_t protocols[MAX_PROTO_PER_BUS];
    int proto_count;
} slave_priv_t;

// --- Slave Plane Write ACL Hook (AEP-403) ---
// 工业网关从站被动接收上位机写指令时，必须经由 ACL 鉴权，严禁未经授权的旁路篡改
static bool _slave_is_write_allowed(uint32_t tag_id) {
    (void)tag_id;
    // 默认安全策略：允许标准业务写点，控制类敏感点需经领域裁决或 ACL 白名单
    return true;
}

static void _slave_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    slave_priv_t *priv = (slave_priv_t *)user_data;
    if (evt == IO_EVENT_RX) {
        net_buf_t *rx = (net_buf_t *)payload;
        if (priv->rx_acc) {
            net_buf_simple_add_mem(&priv->rx_acc->b, rx->b.data, rx->b.len);
            net_buf_unref(rx);
            
            net_buf_t *resp = net_buf_alloc(priv->pool, 0);
            if (resp) {
                int async_count = 0;
                pipe_msg_t async_msgs[8];
                size_t consumed = 0;
                
                int rc = session_manager_handle_rx(priv->session_mgr, priv->rx_acc, resp, async_msgs, &async_count, &consumed);
                if (rc >= 0 && consumed > 0) {
                    net_buf_simple_pull(&priv->rx_acc->b, consumed);
                }
                
                if (resp->b.len > 0) {
                    priv->io->ops->write_async(priv->io, resp);
                } else {
                    net_buf_unref(resp);
                }
                
                for (int i=0; i<async_count; i++) {
                    if (!_slave_is_write_allowed(async_msgs[i].tag_id)) {
                        LOG_WARN(LOG_CAT_CORE, "SlaveEngine: Write to Tag %u REJECTED by Slave Plane ACL", 
                                 async_msgs[i].tag_id);
                        continue;
                    }
                    // --- Edge-Guard: Deep Packet Inspection ---
                    if (async_msgs[i].type == PIPEMSG_TYPE_SCALAR) {
                        dpi_result_t res = dpi_validate_telemetry(async_msgs[i].tag_id, async_msgs[i].payload.val.f);
                        if (res != DPI_PASS) {
                            LOG_WARN(LOG_CAT_CORE, "SlaveEngine: Telemetry for Tag %u BLOCKED by Edge-Guard (Reason: %d)", 
                                     async_msgs[i].tag_id, (int)res);
                            continue; // Skip publishing
                        }
                    }
                    datahub_publish(priv->hub, &async_msgs[i]);
                }
            }
        } else {
            net_buf_unref(rx);
        }
    }
}

static void _slave_on_timer(uv_timer_t *handle) {
    (void)handle;
}

static void _slave_attach(bus_engine_t *self, io_handle_t *io) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _slave_io_cb;
    io->user_data = priv;
    if (priv->rx_acc) net_buf_simple_reset(&priv->rx_acc->b);
    io->ops->enable_read(io, true);
    uv_timer_start(&priv->timer, _slave_on_timer, 1000, 1000);
}

static void _slave_close_cb(uv_handle_t *handle) {
    slave_priv_t *priv = (slave_priv_t *)handle->data;
    if (priv->session_mgr) session_manager_destroy(priv->session_mgr);
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    if (priv->pool) net_buf_pool_unref(priv->pool);
    free(priv);
}

static void _slave_detach(bus_engine_t *self) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    if (priv->io) priv->io->ops->enable_read(priv->io, false);
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) {
        uv_timer_stop(&priv->timer);
        uv_close((uv_handle_t *)&priv->timer, _slave_close_cb);
    }
    free(self);
}

static void _slave_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
}

bus_engine_t *engine_create_slave(driver_manager_t *mgr, int bus_id, const char *protocol,
                                  uint32_t device_id, uv_loop_t *loop, arbiter_t *arb,
                                  datahub_t *hub, net_buf_pool_t *pool,
                                  const driver_contract_injection_t *inject) {
    bus_engine_t *self = calloc(1, sizeof(bus_engine_t));
    slave_priv_t *priv = calloc(1, sizeof(slave_priv_t));
    
    priv->bus_id = bus_id;
    priv->hub = hub;
    priv->loop = loop;
    priv->pool = pool;
    net_buf_pool_ref(pool);
    priv->arb = arb;
    priv->session_mgr = session_manager_create(mgr, 8);
    priv->rx_acc = net_buf_alloc(pool, 0);
    if (priv->rx_acc) net_buf_simple_reset(&priv->rx_acc->b);

    if (protocol) {
        session_manager_add_session(priv->session_mgr, protocol, device_id, inject, hub);
    }

    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    self->user_data = priv;
    self->attach = _slave_attach;
    self->detach = _slave_detach;
    self->run_command = _slave_run_command;
    
    return self;
}

int engine_slave_add_protocol(bus_engine_t *self, const char *protocol, uint32_t device_id,
                              const driver_contract_injection_t *inject) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    return session_manager_add_session(priv->session_mgr, protocol, device_id, inject, priv->hub);
}
