#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "edge_log.h"
#include "edge_request.h"

// 1. Refactor the private struct: remove driver-specific fields
typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    net_buf_pool_t *pool;

    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;
    net_buf_t *rx_acc;
    int bus_id;
} master_priv_t;

// Timeout function now calls the request's on_finish handler
static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        LOG_WARN(LOG_CAT_BUS, "Bus %d: Transaction Timeout [%s]", priv->bus_id,
                 priv->current_cmd->req_id);
        priv->io->ops->enable_read(priv->io, false);

        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish) {
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_TIMEOUT);
        }

        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_TIMEOUT);
        priv->current_cmd = NULL;
    }
}

// 2. Refactor the result processing logic to be generic
static void _process_req_result(master_priv_t *priv, int rc) {
    if (rc == REQ_STATUS_DONE) {
        uv_timer_stop(&priv->timer);
        priv->io->ops->enable_read(priv->io, false);

        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish) {
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_SUCCESS);
        }

        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_SUCCESS);
        priv->current_cmd = NULL;

    } else if (rc == REQ_STATUS_CONTINUE) {
        net_buf_t *tx = net_buf_alloc(priv->pool, 0);
        if (tx && priv->current_cmd->req && priv->current_cmd->req->ops->encode) {
            int encode_rc = priv->current_cmd->req->ops->encode(priv->current_cmd->req, tx);
            if (encode_rc == 0 && tx->b.len > 0) {
                 priv->io->ops->write_async(priv->io, tx);
            } else {
                 net_buf_unref(tx);
            }
        } else if (tx) {
            net_buf_unref(tx);
        }
        uint32_t timeout = priv->current_cmd->req->timeout ? priv->current_cmd->req->timeout : ARBITER_DEFAULT_TIMEOUT_MS;
        uv_timer_start(&priv->timer, _master_on_timeout, timeout, 0);

    } else { // REQ_STATUS_ERROR
        uv_timer_stop(&priv->timer);
        priv->io->ops->enable_read(priv->io, false);

        if (priv->current_cmd->req && priv->current_cmd->req->ops->on_finish) {
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_PROTO_ERROR);
        }

        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_PROTO_ERROR);
        priv->current_cmd = NULL;
    }
}

// 3. Refactor the IO callback to use req->ops->decode
static void _master_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    master_priv_t *priv = (master_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (!priv->current_cmd || !priv->current_cmd->req) return;

    if (evt == IO_EVENT_ERROR) {
        if (priv->current_cmd->req->ops->on_finish) {
            priv->current_cmd->req->ops->on_finish(priv->current_cmd->req, CMD_STATUS_IO_ERROR);
        }
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_IO_ERROR);
        priv->current_cmd = NULL;
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        net_buf_append_bytes(priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        if (priv->current_cmd->req->ops->decode) {
            int rc = priv->current_cmd->req->ops->decode(priv->current_cmd->req, priv->rx_acc);
            if (rc != EDGE_ERR_INCOMPLETE) { // Treat incomplete as "wait for more data"
                 _process_req_result(priv, rc);
            }
        }
    }
}

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _master_io_cb;
    io->user_data = priv;
    io->ops->enable_read(io, false);
}

// 4. Refactor run_command to be generic
static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!priv->io) {
        if (cmd->req && cmd->req->ops->on_finish) cmd->req->ops->on_finish(cmd->req, CMD_STATUS_IO_ERROR);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }
    
    if (!cmd->req || !cmd->req->ops) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        return;
    }

    priv->current_cmd = cmd;
    net_buf_simple_reset(&priv->rx_acc->b);

    if (cmd->req->ops->setup) {
        if (cmd->req->ops->setup(cmd->req, priv) != 0) {
            if (cmd->req->ops->on_finish) cmd->req->ops->on_finish(cmd->req, CMD_STATUS_PROTO_ERROR);
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
            return;
        }
    }
    
    uint32_t timeout = cmd->req->timeout ? cmd->req->timeout : ARBITER_DEFAULT_TIMEOUT_MS;
    uv_timer_start(&priv->timer, _master_on_timeout, timeout, 0);

    net_buf_t *tx = net_buf_alloc(priv->pool, 0);
    if (tx) {
        if (cmd->req->ops->encode) {
            int rc = cmd->req->ops->encode(cmd->req, tx);
            if (rc == 0 && tx->b.len > 0) {
                priv->io->ops->enable_read(priv->io, true);
                priv->io->ops->write_async(priv->io, tx);
            } else {
                 net_buf_unref(tx);
                 if (rc < 0) {
                    uv_timer_stop(&priv->timer);
                    if (cmd->req->ops->on_finish) cmd->req->ops->on_finish(cmd->req, CMD_STATUS_PROTO_ERROR);
                    arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
                 }
            }
        } else {
            net_buf_unref(tx);
        }
    }
}

static void _master_detach(bus_engine_t *self) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    uv_timer_stop(&priv->timer);
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) uv_close((uv_handle_t *)&priv->timer, NULL);
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    if(priv->current_cmd && priv->current_cmd->req && priv->current_cmd->req->ops->teardown) {
        priv->current_cmd->req->ops->teardown(priv->current_cmd->req);
    }
    free(priv);
    free(self);
}

// 5. Refactor factory function to be generic
bus_engine_t *engine_create_master(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                   uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                   net_buf_pool_t *pool,
                                   const driver_contract_injection_t *inject) {
    (void)hub;
    (void)inject;
    (void)protocol;
    (void)mgr;

    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    master_priv_t *priv = calloc(1, sizeof(master_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->pool = pool;
    priv->bus_id = (int)device_id;
    priv->rx_acc = net_buf_alloc(pool, 0);
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    return eng;
}