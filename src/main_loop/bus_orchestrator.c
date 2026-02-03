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
    bus_orch_monitor_tags_t monitor_tags;  // [注入] 监控点位
};

void bus_orchestrator_set_monitor_tags(bus_orchestrator_t *ctx,
                                       const bus_orch_monitor_tags_t *tags) {
    if (ctx && tags) ctx->monitor_tags = *tags;
}

// --- 内存池看门狗 ---
static void _on_pool_audit(uv_timer_t *handle) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)handle->data;
    net_buf_pool_stats_t stats;
    // 审计阈值：10秒 (即如果一个buffer被持有了10秒还未释放，视为泄漏)
    int reclaimed = net_buf_pool_audit(&ctx->pool, 10000, &stats);
    if (reclaimed > 0) {
        LOG_WARN(LOG_CAT_BUS, "Orchestrator: Auto-reclaimed %d leaked buffers!", reclaimed);
    }

    // 发布统计数据到 DataHub (Phase 6 Task 7) - 仅当点位被注入时
    if (ctx->monitor_tags.tag_netbuf_total > 0) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.ts = uv_hrtime();

        msg.tag_id = ctx->monitor_tags.tag_netbuf_total;
        msg.payload.val.i = (int64_t)stats.total_count;
        datahub_publish(ctx->hub, &msg);

        msg.tag_id = ctx->monitor_tags.tag_netbuf_free;
        msg.payload.val.i = (int64_t)stats.free_count;
        datahub_publish(ctx->hub, &msg);

        msg.tag_id = ctx->monitor_tags.tag_netbuf_peak;
        msg.payload.val.i = (int64_t)stats.peak_usage;
        datahub_publish(ctx->hub, &msg);

        msg.tag_id = ctx->monitor_tags.tag_netbuf_leak;
        msg.payload.val.i = (int64_t)stats.leaked_reclaimed;
        datahub_publish(ctx->hub, &msg);
    }
}

// --- 指挥链实现：Arbiter -> Orchestrator -> Engine ---

typedef struct {
    bus_orchestrator_t *orch;
    int bus_id;
} orch_cmd_ctx_t;

static void _on_arbiter_cmd_start(void *user_data) {
    orch_cmd_ctx_t *ctx = (orch_cmd_ctx_t *)user_data;
    bus_orchestrator_t *orch = ctx->orch;
    int bus_id = ctx->bus_id;

    if (bus_id < orch->max_buses && orch->engines[bus_id]) {
        bus_engine_t *eng = orch->engines[bus_id];
        cmd_msg_t *cmd = (cmd_msg_t *)(ctx + 1);
        fprintf(stderr, "[DEBUG][Orchestrator] Dispatching Cmd %s to Engine on Bus %d\n",
                cmd->req_id, bus_id);
        if (eng->run_command) {
            eng->run_command(eng, cmd);
        } else {
            fprintf(stderr,
                    "[ERROR][Orchestrator] Engine on Bus %d has no run_command implementation\n",
                    bus_id);
        }
    } else {
        fprintf(stderr, "[ERROR][Orchestrator] Cannot dispatch: Bus %d has no engine attached\n",
                bus_id);
    }
}

static void _on_arbiter_cmd_cleanup(cmd_msg_t *cmd) {
    if (!cmd) return;
    // 指令包装在 actx 之后，通过 user_data 反向定位
    orch_cmd_ctx_t *actx = (orch_cmd_ctx_t *)cmd->user_data;
    if (actx) {
        free(actx);
    }
}

int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    orch_cmd_ctx_t *actx = malloc(sizeof(orch_cmd_ctx_t) + sizeof(cmd_msg_t));
    actx->orch = ctx;
    actx->bus_id = bus_id;

    cmd_msg_t *wrapped_cmd = (cmd_msg_t *)(actx + 1);
    *wrapped_cmd = *cmd;
    wrapped_cmd->on_start = _on_arbiter_cmd_start;
    wrapped_cmd->on_cleanup = _on_arbiter_cmd_cleanup;
    wrapped_cmd->user_data = actx;

    int rc = arbiter_submit_cmd(ctx->arb, bus_id, wrapped_cmd);
    if (rc != 0) {
        // [Fix] 如果提交失败（通常是因为系统正在关闭），必须立即清理 actx 以防泄漏
        free(actx);
    }
    return rc;
}

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->drv_mgr = mgr;
    ctx->loop = hub->loop;
    // [Nexus] 统一支持到 256 条总线，防止 Matrix 仿真 ID 溢出
    ctx->max_buses = (max_buses > 0 && max_buses <= 256) ? max_buses : 256;
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
                                  uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    bus_engine_t *eng = engine_create_master(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop,
                                             ctx->arb, ctx->hub, &ctx->pool, inject);

    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    // [关键修复] 向 Arbiter 注册总线并获取令牌 (虽然目前令牌是 Opaque)
    arbiter_bus_register(ctx->arb, protocol);

    if (ctx->io_handles[bus_id]) {
        eng->attach(eng, ctx->io_handles[bus_id]);
    }

    return 0;
}

int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;

    bus_engine_t *eng = engine_create_slave(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop,
                                            ctx->arb, ctx->hub, &ctx->pool, inject);

    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    arbiter_bus_register(ctx->arb, protocol);

    if (ctx->io_handles[bus_id]) eng->attach(eng, ctx->io_handles[bus_id]);

    return 0;
}

int bus_orchestrator_add_slave_protocol(bus_orchestrator_t *ctx, int bus_id, const char *protocol,

                                        uint32_t device_id,
                                        const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses || !ctx->engines[bus_id]) return -1;

    return engine_slave_add_protocol(ctx->engines[bus_id], protocol, device_id, inject);
}

int bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (!ctx) return -1;
    // 启动每 10 秒一次的审计
    return uv_timer_start(&ctx->audit_timer, _on_pool_audit, 10000, 10000);
}

struct net_buf_pool *bus_orchestrator_get_pool(bus_orchestrator_t *ctx) {
    return ctx ? &ctx->pool : NULL;
}

static void _on_bus_orch_close(uv_handle_t *handle) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)handle->data;

    fprintf(stderr, "[DEBUG] BusOrchestrator: Final physical memory release.\n");

    if (ctx->io_handles) free(ctx->io_handles);

    if (ctx->engines) free(ctx->engines);

    net_buf_pool_destroy(&ctx->pool);

    free(ctx);
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (ctx) {
        fprintf(stderr, "[DEBUG] BusOrchestrator: Initiating destruction...\n");

        if (!uv_is_closing((uv_handle_t *)&ctx->audit_timer)) {
            uv_timer_stop(&ctx->audit_timer);
        }

        if (ctx->engines) {
            for (int i = 0; i < ctx->max_buses; i++) {
                if (ctx->engines[i]) {
                    fprintf(stderr, "[DEBUG] BusOrchestrator: Detaching engine %d\n", i);

                    ctx->engines[i]->detach(ctx->engines[i]);
                }
            }
        }

        if (ctx->io_handles) {
            for (int i = 0; i < ctx->max_buses; i++) {
                if (ctx->io_handles[i]) {
                    fprintf(stderr, "[DEBUG] BusOrchestrator: Destroying IO handle %d\n", i);

                    ctx->io_handles[i]->ops->destroy(ctx->io_handles[i]);
                }
            }
        }

        if (!uv_is_closing((uv_handle_t *)&ctx->audit_timer)) {
            fprintf(stderr, "[DEBUG] BusOrchestrator: Closing audit timer handle...\n");

            uv_close((uv_handle_t *)&ctx->audit_timer, _on_bus_orch_close);
        }
    }
}

void bus_orchestrator_trigger_reload(bus_orchestrator_t *ctx) {
    if (!ctx) return;
    LOG_INFO(LOG_CAT_BUS, "Bus Orchestrator: Reload triggered.");
    // 标记需要重载，主循环中的调度器会感知此状态
}
