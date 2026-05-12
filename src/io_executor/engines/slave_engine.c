#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "io_executor.h"
#include "session_manager.h"
#include "datahub.h"

#define MAX_SLAVE_PROTOCOLS 4

typedef struct {
    edge_driver_ctx_t *ctx;
    char name[16];
} slave_proto_slot_t;

typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t *pool;
    driver_manager_t *drv_mgr;
    session_manager_t *session_mgr;
    uv_timer_t timer;

    slave_proto_slot_t protocols[MAX_SLAVE_PROTOCOLS];
    int proto_count;
    int last_hit_idx;

    net_buf_t *rx_acc;
    io_handle_t *io;
    int bus_id;
} slave_priv_t;

static void _slave_on_timer(uv_timer_t *handle) {
    slave_priv_t *priv = (slave_priv_t *)handle->data;
    if (!priv->io) return;

    net_buf_t *txs[8];
    int count = session_manager_tick(priv->session_mgr, priv->bus_id, txs, 8);
    for (int i = 0; i < count; i++) {
        priv->io->ops->write_async(priv->io, txs[i]);
        net_buf_unref(txs[i]);
    }
}

static void _slave_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    slave_priv_t *priv = (slave_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR) {
        priv->io = NULL;
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        if (net_buf_simple_tailroom(&priv->rx_acc->b) < rx->b.len) {
            size_t head = net_buf_simple_headroom(&priv->rx_acc->b);
            if (head + net_buf_simple_tailroom(&priv->rx_acc->b) >= rx->b.len) {
                // Shift data to front to reclaim space
                memmove(priv->rx_acc->b.__buf, priv->rx_acc->b.data, priv->rx_acc->b.len);
                priv->rx_acc->b.data = priv->rx_acc->b.__buf;
            } else {
                net_buf_simple_reset(&priv->rx_acc->b);
            }
        }
        net_buf_simple_add_mem(&priv->rx_acc->b, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        while (priv->rx_acc->b.len > 0) {
            bool processed = false;
            uint16_t initial_len = priv->rx_acc->b.len;

            for (int i = 0; i < priv->proto_count; i++) {
                int idx = (i + priv->last_hit_idx) % priv->proto_count;
                slave_proto_slot_t *slot = &priv->protocols[idx];

                net_buf_t *resp = net_buf_alloc(priv->pool, 0);
                if (!resp) break;

                pipe_msg_t async_msgs[16];
                int async_count = 0;
                size_t consumed = 0;

                int rc = slot->ctx->driver->handle_rx(slot->ctx, priv->rx_acc, resp, async_msgs,
                                                      &async_count, &consumed);

                if (rc == 0) {
                    if (resp->b.len > 0) {
                        priv->io->ops->write_async(priv->io, resp);
                    } else {
                        net_buf_unref(resp);
                    }
                    for (int j = 0; j < async_count; j++)
                        datahub_publish(priv->hub, &async_msgs[j]);

                    if (consumed > 0) net_buf_simple_pull(&priv->rx_acc->b, consumed);
                    priv->last_hit_idx = idx;
                    processed = true;
                    break;
                } else {
                    net_buf_unref(resp);
                    if (consumed > 0) {
                        net_buf_simple_pull(&priv->rx_acc->b, consumed);
                        processed = true;
                        break;
                    }
                }
            }

            if (!processed) {
                if (priv->rx_acc->b.len == initial_len) {
                    net_buf_simple_pull(&priv->rx_acc->b, 1);
                } else {
                    break;
                }
            }
        }
    }
}

int engine_slave_add_protocol(bus_engine_t *self, const char *protocol, uint32_t device_id,
                              const driver_contract_injection_t *inject) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    if (priv->proto_count >= MAX_SLAVE_PROTOCOLS) return -1;
    
    edge_driver_ctx_t *ctx = session_manager_get(priv->session_mgr, protocol, device_id);
    if (!ctx) return -1;

    ctx->role = PROTOCOL_ROLE_SLAVE;
    ctx->pool = priv->pool;
    ctx->user_data = priv->hub;

    if (inject) {
        ctx->lookup_by_key = inject->lookup_by_key;
        ctx->lookup_by_tag = inject->lookup_by_tag;
        ctx->meta_list = inject->meta_list;
        ctx->meta_list_len = inject->meta_list_len;
    }

    priv->protocols[priv->proto_count].ctx = ctx;
    strncpy(priv->protocols[priv->proto_count].name, protocol, 15);
    priv->proto_count++;
    return 0;
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
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    slave_priv_t *priv = calloc(1, sizeof(slave_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->hub = hub;
    priv->pool = pool;
    priv->drv_mgr = mgr;
    priv->bus_id = bus_id;
    
    priv->session_mgr = session_manager_create(mgr, 8);
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    priv->rx_acc = net_buf_alloc(pool, 0);
    if (priv->rx_acc) {
        priv->rx_acc->last_owner = "Slave_RX_Acc";
        priv->rx_acc->flags |= NET_BUF_FLAG_PERSISTENT;
        net_buf_simple_reset(&priv->rx_acc->b);
    }
    eng->user_data = priv;
    eng->attach = _slave_attach;
    eng->detach = _slave_detach;
    eng->run_command = _slave_run_command;
    if (protocol && protocol[0] != '\0') {
        engine_slave_add_protocol(eng, protocol, device_id, inject);
    }
    return eng;
}
