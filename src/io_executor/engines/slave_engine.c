#include <stdlib.h>
#include <string.h>

#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"

typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t *pool;
    edge_driver_ctx_t *drv_ctx;
    char proto_name[32];
    net_buf_t *rx_acc;
    io_handle_t *io;
    int bus_id;
} slave_priv_t;

static void _slave_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    slave_priv_t *priv = (slave_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR) return;

    if (evt == IO_EVENT_RX && rx) {
        net_buf_append_bytes(priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        while (priv->rx_acc->b.len > 0) {
            net_buf_t *resp = net_buf_alloc(priv->pool, 0);
            pipe_msg_t async_msgs[16];
            int async_count = 0;
            size_t consumed = 0;

            int rc = priv->drv_ctx->driver->handle_rx(priv->drv_ctx, priv->rx_acc, resp, async_msgs,
                                                      &async_count, &consumed);

            if (rc == 0) {
                if (resp->b.len > 0) priv->io->ops->write_async(priv->io, resp);
                net_buf_unref(resp);
                for (int i = 0; i < async_count; i++) datahub_publish(priv->hub, &async_msgs[i]);
                if (consumed > 0)
                    net_buf_pull(priv->rx_acc, consumed);
                else
                    break;
            } else if (rc == EDGE_ERR_INCOMPLETE) {
                net_buf_unref(resp);
                break;
            } else {
                net_buf_unref(resp);
                if (consumed > 0)
                    net_buf_pull(priv->rx_acc, consumed);
                else
                    net_buf_simple_reset(&priv->rx_acc->b);
                break;
            }
        }
    }
}

static void _slave_attach(bus_engine_t *self, io_handle_t *io) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _slave_io_cb;
    io->user_data = priv;
    io->ops->enable_read(io, true);
}

static void _slave_detach(bus_engine_t *self) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    if (priv->io) priv->io->ops->enable_read(priv->io, false);
    if (priv->drv_ctx) driver_instance_destroy(priv->drv_ctx, priv->proto_name);
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    free(priv);
    free(self);
}

bus_engine_t *engine_create_slave(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                  uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                  net_buf_pool_t *pool) {
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    slave_priv_t *priv = calloc(1, sizeof(slave_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->hub = hub;
    priv->pool = pool;
    strncpy(priv->proto_name, protocol, 31);
    priv->bus_id = (int)device_id;
    priv->rx_acc = net_buf_alloc(pool, 0);
    net_buf_simple_reset(&priv->rx_acc->b);
    priv->drv_ctx = driver_instance_create(mgr, protocol, device_id);
    if (!priv->drv_ctx) {
        free(priv);
        free(eng);
        return NULL;
    }
    priv->drv_ctx->role = PROTOCOL_ROLE_SLAVE;
    eng->user_data = priv;
    eng->attach = _slave_attach;
    eng->detach = _slave_detach;
    return eng;
}
