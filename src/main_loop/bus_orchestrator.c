#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "io_executor.h"

struct bus_orchestrator_s {
    datahub_t *hub;
    arbiter_t *arb;
    driver_manager_t *drv_mgr;
    uv_loop_t *loop;
    net_buf_pool_t pool;
    uv_timer_t audit_timer;  // [Phase 4] 内存池健康检查
    int max_buses;
    io_handle_t **io_handles;
    bus_engine_t **engines;
};

// --- 内存池看门狗 ---
static void _on_pool_audit(uv_timer_t *handle) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)handle->data;
    // 审计阈值：10秒 (即如果一个buffer被持有了10秒还未释放，视为泄漏)
    int reclaimed = net_buf_pool_audit(&ctx->pool, 10000);
    if (reclaimed > 0) {
        LOG_WARN(LOG_CAT_BUS, "Orchestrator: Auto-reclaimed %d leaked buffers!", reclaimed);
    }
}

// ---指挥链实现：Arbiter -> Orchestrator -> Engine ---

typedef struct {
    bus_orchestrator_t *orch;
    int bus_id;
} orch_cmd_ctx_t;

/**
 * @brief 被 Arbiter 调用，标志着一条指令开始在总线上执行
 */
static void _on_arbiter_cmd_start(void *user_data) {
    orch_cmd_ctx_t *ctx = (orch_cmd_ctx_t *)user_data;
    bus_orchestrator_t *orch = ctx->orch;
    int bus_id = ctx->bus_id;

    if (bus_id < orch->max_buses && orch->engines[bus_id]) {
        bus_engine_t *eng = orch->engines[bus_id];
        // 取出当前正在执行的原始命令 (在 context 之后紧跟)
        cmd_msg_t *cmd = (cmd_msg_t *)(ctx + 1);
        eng->run_command(eng, cmd);
    }
}

static void _on_arbiter_cmd_cleanup(cmd_msg_t *cmd) {
    if (cmd->user_data) free(cmd->user_data);
}

int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    // 包装上下文，让 _on_arbiter_cmd_start 知道去哪找 Engine
    orch_cmd_ctx_t *actx = malloc(sizeof(orch_cmd_ctx_t) + sizeof(cmd_msg_t));
    actx->orch = ctx;
    actx->bus_id = bus_id;

    cmd_msg_t *wrapped_cmd = (cmd_msg_t *)(actx + 1);
    *wrapped_cmd = *cmd;
    wrapped_cmd->on_start = _on_arbiter_cmd_start;
    wrapped_cmd->on_cleanup = _on_arbiter_cmd_cleanup;
    wrapped_cmd->user_data = actx;

    return arbiter_submit_cmd(ctx->arb, bus_id, wrapped_cmd);
}

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->drv_mgr = mgr;
    ctx->loop = hub->loop;
    ctx->max_buses = max_buses > 0 ? max_buses : 8;
    ctx->io_handles = calloc((size_t)ctx->max_buses, sizeof(io_handle_t *));
    ctx->engines = calloc((size_t)ctx->max_buses, sizeof(bus_engine_t *));
    net_buf_pool_init(&ctx->pool, (uint16_t)pool_size, (uint16_t)buf_size, 0, NULL, NULL);

    uv_timer_init(ctx->loop, &ctx->audit_timer);
    ctx->audit_timer.data = ctx;

    return ctx;
}

int bus_orchestrator_attach_io(bus_orchestrator_t *ctx, int bus_id, io_handle_t *io) {
    if (ctx && bus_id < ctx->max_buses) {
        ctx->io_handles[bus_id] = io;
        if (ctx->engines[bus_id]) {
            ctx->engines[bus_id]->attach(ctx->engines[bus_id], io);
        }
        return 0;
    }
    return -1;
}

int bus_orchestrator_setup_master(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                  uint32_t device_id) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    bus_engine_t *eng = engine_create_master(ctx->drv_mgr, protocol, device_id, ctx->loop, ctx->arb,
                                             ctx->hub, &ctx->pool);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    if (ctx->io_handles[bus_id]) {
        eng->attach(eng, ctx->io_handles[bus_id]);
    }
    return 0;
}

int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    bus_engine_t *eng = engine_create_slave(ctx->drv_mgr, protocol, device_id, ctx->loop, ctx->arb,
                                            ctx->hub, &ctx->pool);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    if (ctx->io_handles[bus_id]) {
        eng->attach(eng, ctx->io_handles[bus_id]);
    }
    return 0;
}

int bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (!ctx) return -1;
    // 启动每 10 秒一次的审计
    return uv_timer_start(&ctx->audit_timer, _on_pool_audit, 10000, 10000);
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (ctx) {
        uv_timer_stop(&ctx->audit_timer);
        // 注意：这里不执行 uv_close，因为 ctx 即将被释放。
        // 如果 loop 还在运行，这会导致内存泄漏。
        // 正确做法：依赖外部 loop 停止，或者在这里使用 uv_close 并异步 free(ctx)。
        // 鉴于目前的生命周期管理，我们假设 loop 即将停止，或者由外部确保 handle 不再活跃。
        if (!uv_is_closing((uv_handle_t *)&ctx->audit_timer)) {
            uv_close((uv_handle_t *)&ctx->audit_timer, NULL);
        }

        if (ctx->engines) {
            for (int i = 0; i < ctx->max_buses; i++) {
                if (ctx->engines[i]) ctx->engines[i]->detach(ctx->engines[i]);
            }
            free(ctx->engines);
        }
        if (ctx->io_handles) {
            for (int i = 0; i < ctx->max_buses; i++) {
                if (ctx->io_handles[i]) ctx->io_handles[i]->ops->destroy(ctx->io_handles[i]);
            }
            free(ctx->io_handles);
        }
        net_buf_pool_destroy(&ctx->pool);
        free(ctx);
    }
}
