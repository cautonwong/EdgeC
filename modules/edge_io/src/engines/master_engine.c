#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine_priv_internal.h"
#include "edge_log.h"
#include "edge_request.h"
#include "io_executor.h"
#include "session_manager.h"

/**
 * @brief 按总线登记的主站引擎
 *
 * arbiter 的执行器回调只给 bus_id,而引擎是每总线一个实例,靠这张表把 bus_id
 * 翻译回引擎。只在 attach/detach 里增删,uv 单线程,不需要额外锁。
 */
static master_priv_t *g_engine_by_bus[ARBITER_MAX_BUSES];

/**
 * @brief arbiter 自己结算了这条命令(到期扫描),引擎借此清状态并让驱动收尾
 *
 * arbiter_check_timeouts 会在锁内把 owner 摘空、派发下一条,然后才结算并 free。
 * 引擎完全不知情:定时器还挂着、current_cmd 还指着即将释放的内存,驱动侧的
 * on_finish 也没人调用(两阶段 pending 的协议就永远等不到结果)。这里是唯一的
 * 通知点。返回 0 表示不接管释放,arbiter 照常走 on_complete/on_cleanup/free。
 */
static int _on_arbiter_settled(void *ctx, int bus_id, cmd_msg_t *msg, int status) {
    if (bus_id < 0 || bus_id >= ARBITER_MAX_BUSES) return 0;
    master_priv_t *priv = g_engine_by_bus[bus_id];
    // ctx 是注册时带上的 arbiter 身份:进程里可能存在多个 arbiter,不能只按总线号配
    if (!priv || !msg || priv->arb != (arbiter_t *)ctx) return 0;

    // 这个钩子**每次结算都会调**:正常成功/失败路径是引擎自己先调 on_finish 再调
    // arbiter_on_io_done,此时 current_cmd 已经不是 msg 了,不能再叫一遍 on_finish,
    // 否则驱动收到重复结果。只有"引擎还握着它"才说明这份结算是 arbiter 替我做的。
    if (priv->current_cmd != msg) return 0;

    priv->current_cmd = NULL;
    uv_timer_stop(&priv->timer);
    if (priv->io && priv->io->ops) {
        // 和另外两条收尾路径一样先冲一次:队列里的 pending_tx 只有 FLUSH 会释放
        if (priv->io->ops->ioctl) priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        if (priv->io->ops->enable_read) priv->io->ops->enable_read(priv->io, false);
    }
    net_buf_simple_reset(&priv->rx_acc);

    // 驱动侧必须收到这次的失败,否则它那一轮的请求就没有下文了
    if (msg->req && msg->req->ops && msg->req->ops->on_finish) {
        msg->req->ops->on_finish(msg->req, status);
    }
    return 0;
}

/**
 * @brief 事务超时处理
 */
static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        cmd_msg_t *timed_out_cmd = priv->current_cmd;
        if (!arbiter_owns_cmd(priv->arb, priv->bus_id, timed_out_cmd)) {
            // arbiter 已经把这条件结算并 free 掉(真正的收尾在 _on_arbiter_settled),
            // 这里一个字节都不能再读,否则就是 use-after-free
            LOG_WARN(LOG_CAT_BUS, "Bus %d: stale timeout fired after arbiter settled the command",
                     priv->bus_id);
            priv->current_cmd = NULL;
            priv->io->ops->enable_read(priv->io, false);
            return;
        }
        LOG_WARN(LOG_CAT_BUS, "Bus %d: Transaction Timeout [%s]", priv->bus_id,
                 timed_out_cmd->req_id);
        
        // 先摘下来再回调:arbiter 会在 on_io_done 里当场派发下一条指令并换掉
        // current_cmd,不先摘,重入就拿着已释放的 cmd 收尾
        priv->current_cmd = NULL;
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        net_buf_simple_reset(&priv->rx_acc);
        
        if (timed_out_cmd->req && timed_out_cmd->req->ops->on_finish)
            timed_out_cmd->req->ops->on_finish(timed_out_cmd->req, CMD_STATUS_TIMEOUT);
        
        arbiter_on_io_done(priv->arb, priv->bus_id, timed_out_cmd, CMD_STATUS_TIMEOUT);
        
    }
}

/**
 * @brief 驱动执行结果处理引擎
 */
static void _process_req_result(master_priv_t *priv, int rc) {
    cmd_msg_t *cmd = priv->current_cmd;
    if (!cmd) return;
    if (!arbiter_owns_cmd(priv->arb, priv->bus_id, cmd)) {
        // 同上:条件已被 arbiter 结算释放,只清状态,不碰 cmd。这条日志是故意留的:
        // 静默丢一帧结算,两阶段 pending 的协议会把"白读"记成自己的状态错误
        LOG_WARN(LOG_CAT_BUS, "Bus %d: dropped completion for a command the arbiter already settled",
                 priv->bus_id);
        priv->current_cmd = NULL;
        uv_timer_stop(&priv->timer);
        priv->io->ops->enable_read(priv->io, false);
        return;
    }
    // 先摘下来:下面每一路都会回调 arbiter,而 arbiter 会当场派发下一条指令把
    // current_cmd 换成新的;不先摘,重入就拿着已释放的 cmd 收尾(断链 SIGSEGV 根因)
    priv->current_cmd = NULL;

    // 严谨停止超时计时器
    uv_timer_stop(&priv->timer);

    if (rc == REQ_STATUS_DONE) {
        priv->io->ops->enable_read(priv->io, false);
        if (cmd->req && cmd->req->ops->on_finish)
            cmd->req->ops->on_finish(cmd->req, CMD_STATUS_SUCCESS);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_SUCCESS);
    } else if (rc == REQ_STATUS_CONTINUE) {
        net_buf_t *tx = net_buf_alloc(priv->pool, 0);
        if (tx && cmd->req && cmd->req->ops && cmd->req->ops->encode &&
            cmd->req->ops->encode(cmd->req, tx) == 0) {
            // 续帧还要接着收:入口处摘下来的 current_cmd 必须挂回去,否则后续
            // 的 RX/超时都找不到这条命令(多帧协议会静默断在半路)
            priv->current_cmd = cmd;
            // 续展超时计时
            uv_timer_start(&priv->timer, _master_on_timeout, cmd->timeout_ms, 0);
            priv->io->ops->write_async(priv->io, tx);
            net_buf_unref(tx);
        } else {
            if (tx) net_buf_unref(tx);
            priv->current_cmd = cmd;  // 失败收尾走的仍是 _process_req_result,得先挂回去
            _process_req_result(priv, REQ_STATUS_ERROR);
        }
    } else {
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
        priv->io->ops->enable_read(priv->io, false);
        net_buf_simple_reset(&priv->rx_acc);
        if (cmd->req && cmd->req->ops->on_finish)
            cmd->req->ops->on_finish(cmd->req, CMD_STATUS_PROTO_ERROR);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
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

    net_buf_simple_reset(&priv->rx_acc);
    if (priv->io && priv->io->ops && priv->io->ops->ioctl) {
        priv->io->ops->ioctl(priv->io, IO_CTL_FLUSH, NULL);
    }

    if (!cmd->req->ops || !cmd->req->ops->setup || !cmd->req->ops->encode || !cmd->req->ops->decode) {
        LOG_ERROR(LOG_CAT_BUS, "Bus %d: Request ops missing", priv->bus_id);
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        return;
    }

    if (cmd->req->ops->setup(cmd->req, priv) != 0) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        return;
    }

    net_buf_t *tx = net_buf_alloc(priv->pool, 0);
    if (!tx) {
        arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }

    if (cmd->req->ops->encode(cmd->req, tx) == 0 && tx->b.len > 0) {
        // [Safety] 启动原子级单次超时
        // 真正发出去才占 current_cmd:失败路径会在下面直接归还给 arbiter 并释放 cmd,
        // 先占就是给 priv 留一个悬垂指针(断链时 SIGSEGV 的根因)
        priv->current_cmd = cmd;
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

    // 让 arbiter 到期结算时回头通知引擎:超时的唯一权威是 arbiter 的扫描,
    // 引擎只负责清状态并让驱动收尾,避免两份权威互相把对方的命令 free 掉
    if (priv->bus_id >= 0 && priv->bus_id < ARBITER_MAX_BUSES) {
        g_engine_by_bus[priv->bus_id] = priv;
        arb_executor_ops_t ops = {0};
        ops.ctx = priv->arb;
        ops.on_io_done = _on_arbiter_settled;
        arbiter_set_executor(priv->arb, ops);
    }
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
    if (priv->bus_id >= 0 && priv->bus_id < ARBITER_MAX_BUSES &&
        g_engine_by_bus[priv->bus_id] == priv) {
        g_engine_by_bus[priv->bus_id] = NULL;
    }
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) {
        uv_timer_stop(&priv->timer);
        uv_close((uv_handle_t *)&priv->timer, _master_close_cb);
    }
    free(self);
}

bus_engine_t *engine_create_master(driver_manager_t *mgr, int bus_id, const char *protocol,
                                   uint32_t device_id, uv_loop_t *loop, arbiter_t *arb,
                                   datahub_t *hub, net_buf_pool_t *pool) {
    (void)protocol; (void)hub; (void)device_id;
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
