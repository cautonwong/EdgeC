#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine_priv_internal.h"
#include "edge_log.h"
#include "edge_request.h"
#include "io_executor.h"
#include "session_manager.h"

/**
 * @brief 事务超时处理
 */
static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        cmd_msg_t *timed_out_cmd = priv->current_cmd;
        LOG_WARN(LOG_CAT_BUS, "Bus %d: Transaction Timeout [%s]", priv->bus_id,
                 timed_out_cmd->req_id);
        
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        net_buf_simple_reset(&priv->rx_acc);
        
        if (timed_out_cmd->req && timed_out_cmd->req->ops->on_finish)
            timed_out_cmd->req->ops->on_finish(timed_out_cmd->req, CMD_STATUS_TIMEOUT);
        
        arbiter_on_io_done(priv->arb, priv->bus_id, timed_out_cmd, CMD_STATUS_TIMEOUT);
        
        if (priv->current_cmd == timed_out_cmd) {
            priv->current_cmd = NULL;
        }
    }
}

/**
 * @brief 驱动执行结果处理引擎
 */
static void _process_req_result(master_priv_t *priv, int rc) {
    cmd_msg_t *cmd = priv->current_cmd;
    if (!cmd) return;

    // 严谨停止超时计时器
    uv_timer_stop(&priv->timer);

    if (rc == REQ_STATUS_DONE) {
        priv->io->ops->enable_read(priv->io, false);
        if (cmd->req && cmd->req->ops->on_finish)
            cmd->req->ops->on_finish(cmd->req, CMD_STATUS_SUCCESS);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_SUCCESS);
        if (priv->current_cmd == cmd) priv->current_cmd = NULL;
    } else if (rc == REQ_STATUS_CONTINUE) {
        net_buf_t *tx = net_buf_alloc(priv->pool, 0);
        if (tx && cmd->req->ops->encode(cmd->req, tx) == 0) {
            // 续展超时计时
            uv_timer_start(&priv->timer, _master_on_timeout, cmd->timeout_ms, 0);
            priv->io->ops->write_async(priv->io, tx);
            net_buf_unref(tx);
        } else {
            if (tx) net_buf_unref(tx);
            _process_req_result(priv, REQ_STATUS_ERROR);
        }
    } else {
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        net_buf_simple_reset(&priv->rx_acc);
        if (cmd->req && cmd->req->ops->on_finish)
            cmd->req->ops->on_finish(cmd->req, CMD_STATUS_PROTO_ERROR);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        if (priv->current_cmd == cmd) priv->current_cmd = NULL;
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
        if (net_buf_simple_tailroom(&priv->rx_acc) < rx->b.len) {
            LOG_WARN(LOG_CAT_BUS, "Bus %d: RX Accumulator Overflow, resetting...", priv->bus_id);
            net_buf_simple_reset(&priv->rx_acc);
        }
        net_buf_simple_add_mem(&priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        while (priv->current_cmd && priv->current_cmd->req && priv->rx_acc.len > 0) {
            size_t consumed = 0;
            net_buf_t wrap = {0};
            wrap.pool = priv->pool;
            memcpy(&wrap.b, &priv->rx_acc, sizeof(struct net_buf_simple));
            
            int rc = priv->current_cmd->req->ops->decode(priv->current_cmd->req, &wrap, &consumed);
            if (consumed > 0) {
                net_buf_simple_pull(&priv->rx_acc, consumed);
            }
            if (rc != EDGE_ERR_INCOMPLETE) {
                _process_req_result(priv, rc);
                break;
            }
            if (consumed == 0) {
                break;
            }
        }
    }
}

static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!priv || !priv->io || !cmd->req) {
        LOG_ERROR(LOG_CAT_BUS, "Bus %d: Run command failed - IO/Request NULL", priv ? priv->bus_id : -1);
        arbiter_on_io_done(priv ? priv->arb : NULL, priv ? priv->bus_id : -1, cmd, CMD_STATUS_IO_ERROR);
        return;
    }

    priv->current_cmd = cmd;
    net_buf_simple_reset(&priv->rx_acc);
    if (priv->io && priv->io->ops && priv->io->ops->ioctl) {
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
    }

    if (!cmd->req->ops || !cmd->req->ops->setup || !cmd->req->ops->encode || !cmd->req->ops->decode) {
        LOG_ERROR(LOG_CAT_BUS, "Bus %d: Request ops missing", priv->bus_id);
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
        // [Safety] 启动原子级单次超时
        uv_timer_start(&priv->timer, _master_on_timeout, cmd->timeout_ms, 0);
        priv->io->ops->enable_read(priv->io, true);
        priv->io->ops->write_async(priv->io, tx);
        net_buf_unref(tx);
    } else {
        if (tx) net_buf_unref(tx);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
    }
}

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _master_io_cb;
    io->user_data = priv;
    LOG_INFO(LOG_CAT_BUS, "Bus %d: Physical I/O attached (Type: %d)", priv->bus_id, io->ops ? 0 : -1);
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
    priv->session_mgr = session_manager_create(mgr, 128); // [Professional] 提升通道设备容量，支持 2000 只表均匀分布
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;
    
    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    return eng;
}
