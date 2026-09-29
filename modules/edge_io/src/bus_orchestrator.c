#include <stdlib.h>
#include <string.h>
#include "bus_orchestrator.h"
#include "engines/engine_priv_internal.h"
#include "edge_log.h"
#include "net_buf.h"

// --- Forward Declarations ---
static void _on_pool_audit(uv_timer_t *handle);

struct bus_orchestrator_s {
    datahub_t *hub;
    arbiter_t *arb;
    driver_manager_t *drv_mgr;
    uv_loop_t *loop;
    net_buf_pool_t *pool;

    int max_buses;
    bus_engine_t *engines[ARBITER_MAX_BUSES];
    io_handle_t *io_handles[ARBITER_MAX_BUSES];

    uv_timer_t audit_timer;
};

// 【实事求是】全局编排器实例，用于解决 C 语言异步回调中的对象找回难题
static bus_orchestrator_t *g_active_orch = NULL;

// 【绝对安全】回调包装器：确保 self 指针正确传入驱动引擎
static void _engine_on_start_wrapper(void *user_data) {
    cmd_msg_t *cmd = (cmd_msg_t *)user_data;
    int bus_id = (int)cmd->signature; // 我们利用 signature 存放 bus_id
    
    if (g_active_orch && bus_id >= 0 && bus_id < ARBITER_MAX_BUSES) {
        bus_engine_t *eng = g_active_orch->engines[bus_id];
        if (eng && eng->run_command) {
            // 【核心修正】显式传递 self (eng) 指针，消灭 Segfault
            // 不再非法篡改 priv->device_id，该字段应由 cmd_msg 直接携带
            eng->run_command(eng, cmd);
        }
    }
_Static_assert(sizeof(struct bus_orchestrator_s) <= sizeof(bus_orchestrator_storage_t),
               "bus_orchestrator_s size exceeds bus_orchestrator_storage_t");

static bus_orchestrator_storage_t g_default_orch_storage;

bus_orchestrator_t *bus_orchestrator_create_in(bus_orchestrator_storage_t *storage,
                                               datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                               int max_buses, uint32_t pool_size, uint32_t buf_size) {
    if (!storage || !hub) return NULL;
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)storage->raw;
    memset(ctx, 0, sizeof(*ctx));
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->drv_mgr = mgr;
    ctx->loop = hub->loop;
    ctx->max_buses = (max_buses > ARBITER_MAX_BUSES) ? ARBITER_MAX_BUSES : max_buses;
    ctx->pool = net_buf_pool_new(pool_size, (uint16_t)buf_size, 0);

    g_active_orch = ctx; // 注册全局活动实例

    uv_timer_init(ctx->loop, &ctx->audit_timer);
    ctx->audit_timer.data = ctx;
    return ctx;
}

bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, driver_manager_t *mgr,
                                            int max_buses, uint32_t pool_size, uint32_t buf_size) {
    return bus_orchestrator_create_in(&g_default_orch_storage, hub, arb, mgr, max_buses, pool_size, buf_size);
}

int bus_orchestrator_attach_io(bus_orchestrator_t *ctx, int bus_id, io_handle_t *io) {
    if (!ctx || bus_id < 0 || bus_id >= ctx->max_buses || !io) return -1;
    
    // 💡 [Nexus Fix] 记录 IO 句柄，即使 Engine 还没准备好
    ctx->io_handles[bus_id] = io;
    
    if (ctx->engines[bus_id]) {
        if (ctx->engines[bus_id]->attach) {
            printf("🔗 [ORCHESTRATOR] Bus %d: Early-Attached IO %p to Engine %p\n", 
                   bus_id, (void*)io, (void*)ctx->engines[bus_id]);
            ctx->engines[bus_id]->attach(ctx->engines[bus_id], io);
        }
    } else {
        printf("⏳ [ORCHESTRATOR] Bus %d: IO %p registered, waiting for Engine...\n", 
               bus_id, (void*)io);
    }
    return 0;
}

int bus_orchestrator_setup_master(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                  uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id < 0 || bus_id >= ctx->max_buses) return -1;
    bus_engine_t *eng = engine_create_master(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop, ctx->arb, ctx->hub, ctx->pool, inject);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    arbiter_bus_register(ctx->arb, protocol);
    if (ctx->io_handles[bus_id]) eng->attach(eng, ctx->io_handles[bus_id]);
    return 0;
}

int bus_orchestrator_setup_slave(bus_orchestrator_t *ctx, int bus_id, const char *protocol,
                                 uint32_t device_id, const driver_contract_injection_t *inject) {
    if (!ctx || bus_id < 0 || bus_id >= ctx->max_buses) return -1;
    bus_engine_t *eng = engine_create_slave(ctx->drv_mgr, bus_id, protocol, device_id, ctx->loop, ctx->arb, ctx->hub, ctx->pool, inject);
    if (!eng) return -1;

    ctx->engines[bus_id] = eng;
    arbiter_bus_register(ctx->arb, protocol);
    if (ctx->io_handles[bus_id]) eng->attach(eng, ctx->io_handles[bus_id]);
    return 0;
}

io_handle_t *bus_orchestrator_get_io(bus_orchestrator_t *ctx, int bus_id) {
    if (ctx && bus_id >= 0 && bus_id < ctx->max_buses) {
        return ctx->io_handles[bus_id];
    }
    return NULL;
}

int bus_orchestrator_submit_cmd(bus_orchestrator_t *ctx, int bus_id, cmd_msg_t *cmd) {
    if (!ctx || bus_id < 0 || bus_id >= ctx->max_buses || !cmd) return -1;

    // 绑定包装后的回调
    cmd->signature = (uint32_t)bus_id;
    cmd->on_start = _engine_on_start_wrapper;

    return arbiter_submit_cmd(ctx->arb, bus_id, cmd);
}

int bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (!ctx) return -1;
    return uv_timer_start(&ctx->audit_timer, _on_pool_audit, 10000, 10000);
}

datahub_t *bus_orchestrator_get_hub(bus_orchestrator_t *ctx) {
    return ctx ? ctx->hub : NULL;
}

struct net_buf_pool *bus_orchestrator_get_pool(bus_orchestrator_t *ctx) {
    return ctx ? ctx->pool : NULL;
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (!ctx) return;
    if (g_active_orch == ctx) g_active_orch = NULL;
}

static void _on_pool_audit(uv_timer_t *handle) { (void)handle; }
