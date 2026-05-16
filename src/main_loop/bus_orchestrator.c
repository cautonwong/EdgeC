#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

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
    net_buf_pool_t *pool; // [Nexus] Standalone heap pool
    uv_timer_t audit_timer;
    int max_buses;
    io_handle_t **io_handles;
    bus_engine_t **engines;
    bus_orch_monitor_tags_t monitor_tags;
    atomic_int ref_count; // [Nexus] Lifecycle management
};

void bus_orchestrator_set_monitor_tags(bus_orchestrator_t *ctx,
                                       const bus_orch_monitor_tags_t *tags) {
    if (ctx && tags) ctx->monitor_tags = *tags;
}

static void _bus_orch_unref(bus_orchestrator_t *ctx) {
    if (atomic_fetch_sub(&ctx->ref_count, 1) == 1) {
        fprintf(stderr, "[DEBUG] BusOrchestrator: Final physical memory release.\n");
        if (ctx->io_handles) free(ctx->io_handles);
        if (ctx->engines) free(ctx->engines);
        if (ctx->pool) net_buf_pool_unref(ctx->pool);
        free(ctx);
    }
}

static void _on_bus_orch_close(uv_handle_t *handle) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)handle->data;
    _bus_orch_unref(ctx);
}

// --- 内存池看门狗 ---
static void _on_pool_audit(uv_timer_t *handle) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)handle->data;
    if (!ctx->pool) return;
    net_buf_pool_stats_t stats;
    int reclaimed = net_buf_pool_audit(ctx->pool, 10000, &stats);
    if (reclaimed > 0) {
        LOG_WARN(LOG_CAT_BUS, "Orchestrator: Auto-reclaimed %d leaked buffers!", reclaimed);
    }

    if (ctx->monitor_tags.tag_netbuf_total > 0) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.ts = uv_hrtime();
        msg.tag_id = ctx->monitor_tags.tag_netbuf_total;
        msg.payload.val.i = (int64_t)stats.total_count;
        datahub_publish(ctx->hub, &msg);
    }
}

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb,
                                           driver_manager_t *mgr, int max_buses,
                                           uint32_t pool_slots, uint32_t pool_buf_size) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->drv_mgr = mgr;
    ctx->max_buses = max_buses;
    ctx->io_handles = calloc((size_t)max_buses, sizeof(io_handle_t *));
    ctx->engines = calloc((size_t)max_buses, sizeof(bus_engine_t *));
    atomic_init(&ctx->ref_count, 1);

    ctx->pool = net_buf_pool_new(pool_slots, (uint16_t)pool_buf_size, 0);
    if (!ctx->pool) {
        free(ctx->io_handles);
        free(ctx->engines);
        free(ctx);
        return NULL;
    }

    uv_timer_init(hub->loop, &ctx->audit_timer);
    ctx->audit_timer.data = ctx;
    ctx->loop = hub->loop;

    return ctx;
}

int bus_orchestrator_setup_master(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                  uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;
    bus_engine_t *eng = engine_create_master(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop, ctx->arb, ctx->hub, ctx->pool, inject);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    arbiter_bus_register(ctx->arb, protocol);
    if (ctx->io_handles[bus_id]) eng->attach(eng, ctx->io_handles[bus_id]);
    return 0;
}

int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses) return -1;
    bus_engine_t *eng = engine_create_slave(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop, ctx->arb, ctx->hub, ctx->pool, inject);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    arbiter_bus_register(ctx->arb, protocol);
    if (ctx->io_handles[bus_id]) eng->attach(eng, ctx->io_handles[bus_id]);
    return 0;
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

io_handle_t *bus_orchestrator_get_io(bus_orchestrator_t *ctx, int bus_id) {
    if (ctx && bus_id >= 0 && bus_id < ctx->max_buses) {
        return ctx->io_handles[bus_id];
    }
    return NULL;
}

int bus_orchestrator_add_slave_protocol(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                        uint32_t device_id,
                                        const driver_contract_injection_t *inject) {
    if (!ctx || bus_id >= ctx->max_buses || !ctx->engines[bus_id]) return -1;
    return engine_slave_add_protocol(ctx->engines[bus_id], protocol, device_id, inject);
}

int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd) {
    if (!ctx || bus_id >= ctx->max_buses || !cmd) return -1;
    return arbiter_submit_cmd(ctx->arb, bus_id, cmd);
}

int bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (!ctx) return -1;
    return uv_timer_start(&ctx->audit_timer, _on_pool_audit, 10000, 10000);
}

void bus_orchestrator_trigger_reload(bus_orchestrator_t *ctx) {
    (void)ctx;
    LOG_INFO(LOG_CAT_BUS, "BusOrchestrator: Triggered reload (Stub)");
}

struct net_buf_pool *bus_orchestrator_get_pool(bus_orchestrator_t *ctx) {
    return ctx ? ctx->pool : NULL;
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (!ctx) return;
    fprintf(stderr, "[DEBUG] BusOrchestrator: Initiating destruction...\n");

    if (ctx->engines) {
        for (int i = 0; i < ctx->max_buses; i++) {
            if (ctx->engines[i]) {
                fprintf(stderr, "[DEBUG] BusOrchestrator: Detaching engine %d\n", i);
                ctx->engines[i]->detach(ctx->engines[i]);
                ctx->engines[i] = NULL;
            }
        }
    }

    if (ctx->io_handles) {
        for (int i = 0; i < ctx->max_buses; i++) {
            if (ctx->io_handles[i]) {
                fprintf(stderr, "[DEBUG] BusOrchestrator: Destroying IO handle %d\n", i);
                if (ctx->io_handles[i]->ops && ctx->io_handles[i]->ops->destroy) {
                    ctx->io_handles[i]->ops->destroy(ctx->io_handles[i]);
                }
            }
        }
    }

    if (!uv_is_closing((uv_handle_t *)&ctx->audit_timer)) {
        fprintf(stderr, "[DEBUG] BusOrchestrator: Closing audit timer handle...\n");
        uv_close((uv_handle_t *)&ctx->audit_timer, _on_bus_orch_close);
    } else {
        _bus_orch_unref(ctx);
    }
}
