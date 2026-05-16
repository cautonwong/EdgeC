#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "edge_log.h"
#include "edge_request.h"
#include "io_executor.h"
#include "session_manager.h"

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
    session_manager_t *session_mgr;
} master_priv_t;

static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        LOG_WARN(LOG_CAT_BUS, "Bus %d: Transaction Timeout [%s]", priv->bus_id,
                 priv->current_cmd->req_id);
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish)
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_TIMEOUT);
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_TIMEOUT);
        priv->current_cmd = NULL;
    }
    net_buf_t *heartbeats[8];
    int count = session_manager_tick(priv->session_mgr, priv->bus_id, heartbeats, 8);
    for (int i = 0; i < count; i++) {
        priv->io->ops->write_async(priv->io, heartbeats[i]);
        net_buf_unref(heartbeats[i]);
    }
    uv_timer_start(&priv->timer, _master_on_timeout, ARBITER_DEFAULT_TIMEOUT_MS, 0);
}

static void _process_req_result(master_priv_t *priv, int rc) {
    if (rc == REQ_STATUS_DONE) {
        priv->io->ops->enable_read(priv->io, false);
        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish)
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_SUCCESS);
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_SUCCESS);
        priv->current_cmd = NULL;
    } else if (rc == REQ_STATUS_CONTINUE) {
        net_buf_t *tx = net_buf_alloc(priv->pool, 0);
        if (tx && priv->current_cmd->req->ops->encode(priv->current_cmd->req, tx) == 0) {
            priv->io->ops->write_async(priv->io, tx);
            net_buf_unref(tx);
        } else {
            if (tx) net_buf_unref(tx);
            _process_req_result(priv, REQ_STATUS_ERROR);
        }
    } else {
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish)
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_PROTO_ERROR);
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_PROTO_ERROR);
        priv->current_cmd = NULL;
    }
}

static void _master_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    master_priv_t *priv = (master_priv_t *)user_data;
    if (!priv) {
        if (evt == IO_EVENT_RX && payload) net_buf_unref((net_buf_t *)payload);
        return;
    }
    if (evt == IO_EVENT_ERROR) {
        if (priv->current_cmd) _process_req_result(priv, REQ_STATUS_ERROR);
        return;
    }
    if (evt == IO_EVENT_RX && payload) {
        net_buf_t *rx = (net_buf_t *)payload;
        if (net_buf_simple_tailroom(&priv->rx_acc) < rx->b.len) net_buf_simple_reset(&priv->rx_acc);
        net_buf_simple_add_mem(&priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);
        if (priv->current_cmd && priv->current_cmd->req) {
            size_t consumed = 0;
            net_buf_t wrap = {0};
            wrap.pool = priv->pool;
            memcpy(&wrap.b, &priv->rx_acc, sizeof(struct net_buf_simple));
            int rc = priv->current_cmd->req->ops->decode(priv->current_cmd->req, &wrap, &consumed);
            if (consumed > 0) net_buf_simple_pull(&priv->rx_acc, consumed);
            if (rc != EDGE_ERR_INCOMPLETE) _process_req_result(priv, rc);
        }
    }
}

static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!priv || !priv->io || !cmd->req) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }
    priv->current_cmd = cmd;
    net_buf_simple_reset(&priv->rx_acc);
    if (!cmd->req->ops || !cmd->req->ops->setup || !cmd->req->ops->encode || !cmd->req->ops->decode) {
        _process_req_result(priv, REQ_STATUS_ERROR);
        return;
    }
    if (cmd->req->ops->setup(cmd->req, priv) != 0) {
        _process_req_result(priv, REQ_STATUS_ERROR);
        return;
    }
    net_buf_t *tx = net_buf_alloc(priv->pool, 0);
    if (!tx) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }
    if (cmd->req->ops->encode(cmd->req, tx) == 0 && tx->b.len > 0) {
        priv->io->ops->enable_read(priv->io, true);
        priv->io->ops->write_async(priv->io, tx);
        net_buf_unref(tx);
    } else {
        net_buf_unref(tx);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
    }
}

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _master_io_cb;
    io->user_data = priv;
}

static void _master_close_cb(uv_handle_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->session_mgr) session_manager_destroy(priv->session_mgr);
    if (priv->pool) net_buf_pool_unref(priv->pool);
    free(priv);
}

static void _master_detach(bus_engine_t *self) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) {
        uv_timer_stop(&priv->timer);
        uv_close((uv_handle_t *)&priv->timer, _master_close_cb);
    }
    free(self);
}

bus_engine_t *engine_create_master(driver_manager_t *mgr, int bus_id, const char *protocol,
                                   uint32_t device_id, uv_loop_t *loop, arbiter_t *arb,
                                   datahub_t *hub, net_buf_pool_t *pool,
                                   const driver_contract_injection_t *inject) {
    (void)protocol; (void)hub; (void)inject; (void)device_id;
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    master_priv_t *priv = calloc(1, sizeof(master_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->pool = pool;
    net_buf_pool_ref(pool);
    priv->bus_id = bus_id;
    priv->rx_acc.data = priv->rx_static_data;
    priv->rx_acc.__buf = priv->rx_static_data;
    priv->rx_acc.size = 4096;
    priv->rx_acc.len = 0;
    priv->session_mgr = session_manager_create(mgr, 32);
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;
    uv_timer_start(&priv->timer, _master_on_timeout, 1000, 1000);
    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    return eng;
}
