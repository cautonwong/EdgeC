#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"

typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    net_buf_pool_t *pool;
    driver_manager_t *drv_mgr;

    edge_driver_ctx_t *drv_ctx;
    char current_proto[16];

    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;
    net_buf_t *rx_acc;
    int bus_id;
} master_priv_t;

static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        LOG_WARN(LOG_CAT_BUS, "Bus %d: Transaction Timeout [%s]", priv->bus_id,
                 priv->current_cmd->req_id);
        priv->io->ops->enable_read(priv->io, false);
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_TIMEOUT);
        priv->current_cmd = NULL;
    }
}

// 核心：处理驱动层返回的指令，决定是结束、继续发送、还是等待
static void _process_driver_result(master_priv_t *priv, int rc, net_buf_t *resp, pipe_msg_t *msgs,
                                   int count, size_t consumed) {
    // 1. 如果驱动产生了响应数据（不管是中间步骤还是最终响应），立即发送
    if (resp && resp->b.len > 0) {
        priv->io->ops->write_async(priv->io, resp);
    } else if (resp) {
        net_buf_unref(resp);
    }

    // 2. 消耗已处理的 RX 数据
    if (consumed > 0) net_buf_pull(priv->rx_acc, consumed);

    // 3. 根据驱动返回的状态码决定调度
    if (rc == 0) {
        // [事务真正完成]
        uv_timer_stop(&priv->timer);
        priv->io->ops->enable_read(priv->io, false);

        // 此处不再硬编码 publish，而是通过仲裁器转发给 cmd->on_complete
        // 发起者会在 on_complete 中处理 msgs 数组（数据可能通过 cmd->params 传递）
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_SUCCESS);
        priv->current_cmd = NULL;

    } else if (rc == 1) {  // 约定 1 为 EDGE_ERR_CONTINUE
        // [事务中：需要立即发送下一包]
        // 逻辑已在上面 write_async 完成，只需重启计时器，继续保持读取开启
        uv_timer_start(&priv->timer, _master_on_timeout,
                       priv->current_cmd->timeout_ms ? priv->current_cmd->timeout_ms : 1000, 0);

    } else if (rc == -2) {  // 约定 -2 为 EDGE_ERR_INCOMPLETE
        // [半包：继续等待]
        // 保持现状，等待下一次 _master_io_cb
    } else {
        // [协议错误]
        uv_timer_stop(&priv->timer);
        priv->io->ops->enable_read(priv->io, false);
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_PROTO_ERROR);
        priv->current_cmd = NULL;
    }
}

static void _master_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    master_priv_t *priv = (master_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR && priv->current_cmd) {
        arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_IO_ERROR);
        priv->current_cmd = NULL;
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        net_buf_append_bytes(priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        if (!priv->drv_ctx) return;

        // 如果是透传指令且动作为 RAW，驱动可以直接将 rx_acc 的内容复制到 resp
        pipe_msg_t msgs[16];
        int count = 0;
        size_t consumed = 0;
        net_buf_t *resp = net_buf_alloc(priv->pool, 0);

        int rc = priv->drv_ctx->driver->decode_response(priv->drv_ctx, priv->rx_acc, resp, msgs,
                                                        &count, &consumed);

        _process_driver_result(priv, rc, resp, msgs, count, consumed);
    }
}

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _master_io_cb;
    io->user_data = priv;
    io->ops->enable_read(io, false);
}

static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!priv->io) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }

    // 动态加载驱动（如果协议变了）
    if (cmd->protocol[0] != '\0' && strcmp(cmd->protocol, priv->current_proto) != 0) {
        if (priv->drv_ctx) driver_instance_destroy(priv->drv_ctx, priv->current_proto);
        priv->drv_ctx = driver_instance_create(priv->drv_mgr, cmd->protocol, cmd->tag_id);
        if (!priv->drv_ctx) {
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
            return;
        }
        priv->drv_ctx->role = PROTOCOL_ROLE_MASTER;
        priv->drv_ctx->pool = priv->pool;
        strncpy(priv->current_proto, cmd->protocol, 15);
        net_buf_simple_reset(&priv->rx_acc->b);
    }

    priv->current_cmd = cmd;

    if (cmd->phy_params && cmd->phy_params->baud > 0) {
        priv->io->ops->ioctl(priv->io, IO_CTL_SET_BAUD, &cmd->phy_params->baud);
    }

    uv_timer_start(&priv->timer, _master_on_timeout, cmd->timeout_ms ? cmd->timeout_ms : 1000, 0);

    // 封包并发送第一回合请求
    net_buf_t *tx = net_buf_alloc(priv->pool, 0);
    if (tx) {
        // [透传优化] 如果是 RAW 动作，驱动可以直接把 cmd->params 的内容搬运到 tx
        int rc = priv->drv_ctx->driver->encode_request(priv->drv_ctx, &cmd->tag_id, 1, tx);
        if (rc == 0 || rc == 1) {  // 0: 完成, 1: 事务进行中
            priv->io->ops->enable_read(priv->io, true);
            priv->io->ops->write_async(priv->io, tx);
        } else {
            net_buf_unref(tx);
            uv_timer_stop(&priv->timer);
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        }
    }
}

static void _master_detach(bus_engine_t *self) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    uv_timer_stop(&priv->timer);
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) uv_close((uv_handle_t *)&priv->timer, NULL);
    if (priv->drv_ctx) driver_instance_destroy(priv->drv_ctx, priv->current_proto);
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    free(priv);
    free(self);
}

bus_engine_t *engine_create_master(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                   uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                   net_buf_pool_t *pool,
                                   const driver_contract_injection_t *inject) {
    (void)hub;
    (void)inject;
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    master_priv_t *priv = calloc(1, sizeof(master_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->pool = pool;
    priv->drv_mgr = mgr;
    priv->bus_id = (int)device_id;
    priv->rx_acc = net_buf_alloc(pool, 0);
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    if (protocol && protocol[0] != '\0') {
        priv->drv_ctx = driver_instance_create(mgr, protocol, device_id);
        if (priv->drv_ctx) {
            priv->drv_ctx->role = PROTOCOL_ROLE_MASTER;
            priv->drv_ctx->pool = priv->pool;
            strncpy(priv->current_proto, protocol, 15);
        }
    }

    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    return eng;
}
