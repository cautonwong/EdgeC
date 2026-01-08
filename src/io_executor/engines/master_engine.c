#include <stdlib.h>

#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"

typedef struct {
    bus_engine_t base;  // 继承

    // Dependencies
    uv_loop_t *loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t *pool;

    // State
    edge_driver_ctx_t *drv_ctx;
    char proto_name[32];
    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;
    int bus_id;
    bool is_busy;
} master_priv_t;

// Forward declarations
static void _master_detach(bus_engine_t *self);
static void _master_on_timeout(uv_timer_t *handle);

// --- IO Callback ---
static void _master_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    master_priv_t *priv = (master_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR) {
        LOG_ERROR(LOG_CAT_BUS, "Bus %d IO Error", priv->bus_id);
        if (priv->current_cmd) {
            arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd);
            priv->current_cmd = NULL;
        }
        priv->io->ops->enable_read(priv->io, false);
        return;
    }

    if (evt == IO_EVENT_TX_DONE) {
        // 发送完成，开始计算超时 (也可以在 run_command 时开始)
        // 某些协议要求从发送结束开始计时
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        if (!priv->current_cmd || !priv->drv_ctx->driver->decode_response) {
            net_buf_unref(rx);
            return;
        }

        // 解析响应
        pipe_msg_t msgs[16];  // 假设一批最多16个点
        int count = 0;
        int rc = priv->drv_ctx->driver->decode_response(priv->drv_ctx, rx, msgs, &count);
        net_buf_unref(rx);  // 消费完毕

        if (rc == 0) {
            // 成功解析：发布数据
            for (int i = 0; i < count; i++) {
                datahub_publish(priv->hub, &msgs[i]);
            }

            // 停止超时计时器
            uv_timer_stop(&priv->timer);
            priv->io->ops->enable_read(priv->io, false);  // 关闭监听节能/防噪

            // 通知仲裁器任务完成
            if (priv->current_cmd) {
                arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd);
                priv->current_cmd = NULL;
            }
        } else {
            // 解析失败（可能是分包或噪音），这里简单处理为继续等待，直到超时
            // 生产环境应处理分包逻辑
            LOG_WARN(LOG_CAT_BUS, "Bus %d decode failed: %d", priv->bus_id, rc);
        }
    }
}

// --- Engine Interfaces ---

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    // 注册回调：Priv 作为 user_data
    io->cb = _master_io_cb;
    io->user_data = priv;

    // 默认关闭读
    io->ops->enable_read(io, false);
}

static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;

    if (!priv->io || !priv->drv_ctx) return;

    priv->current_cmd = cmd;

    // 1. 设置超时

    uint64_t timeout = cmd->timeout_ms > 0 ? cmd->timeout_ms : 1000;

    uv_timer_start(&priv->timer, _master_on_timeout, timeout, 0);

    // 2. 编码请求

    // 使用来自 cmd 的 tag_id 开启采集

    net_buf_t *tx = net_buf_alloc(priv->pool, 0);

    if (tx) {
        uint32_t tags[1] = {cmd->tag_id};

        int rc = priv->drv_ctx->driver->encode_request(priv->drv_ctx, tags, 1, tx);

        if (rc == 0) {
            // 3. 开启监听

            priv->io->ops->enable_read(priv->io, true);

            // 4. 发送

            priv->io->ops->write_async(priv->io, tx);

        } else {
            net_buf_unref(tx);

            uv_timer_stop(&priv->timer);

            if (priv->arb) arbiter_on_io_done(priv->arb, priv->bus_id, cmd);
        }
    }
}

static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;

    if (priv->current_cmd) {
        LOG_WARN(LOG_CAT_BUS, "Bus %d Timeout on cmd %s", priv->bus_id, priv->current_cmd->req_id);

        priv->io->ops->enable_read(priv->io, false);

        // Notify Arbiter

        if (priv->arb) arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd);

        priv->current_cmd = NULL;
    }
}

static void _master_detach(bus_engine_t *self) {
    master_priv_t *priv = (master_priv_t *)self->user_data;

    uv_timer_stop(&priv->timer);

    if (!uv_is_closing((uv_handle_t *)&priv->timer)) {
        uv_close((uv_handle_t *)&priv->timer, NULL);
    }

    if (priv->drv_ctx) {
        driver_instance_destroy(priv->drv_ctx, priv->proto_name);
    }

    free(priv);

    free(self);
}

// --- Factory ---

bus_engine_t *engine_create_master(driver_manager_t *mgr, const char *protocol, uint32_t device_id,

                                   uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                   net_buf_pool_t *pool) {
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));

    master_priv_t *priv = calloc(1, sizeof(master_priv_t));

    priv->loop = loop;

    priv->arb = arb;

    priv->hub = hub;

    priv->pool = pool;

    strncpy(priv->proto_name, protocol, sizeof(priv->proto_name) - 1);

    priv->bus_id = (int)device_id;

    // 初始化 Timer
    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    // 创建驱动实例
    priv->drv_ctx = driver_instance_create(mgr, protocol, device_id);
    if (!priv->drv_ctx) {
        free(priv);
        free(eng);
        return NULL;
    }
    priv->drv_ctx->role = PROTOCOL_ROLE_MASTER;

    // 绑定接口
    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    // on_data not used directly by orchestrator anymore

    return eng;
}
