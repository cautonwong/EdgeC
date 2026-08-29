#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contract_registry.h"
#include "datahub.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "edge_service.h"
#include "io_executor.h"
#include "sqlite_service.h"
#include "stream_engine.h"

// --- Global Contexts ---
dcu_inventory_t g_dcu_inventory;
dcu_poller_t g_dcu_poller;
dcu_storage_t g_dcu_storage;

static dcu_event_obj_t g_evt_3000;
static dcu_event_obj_t g_evt_3005;

static service_t* g_running_svc = NULL;
void* g_running_se = NULL;  // Exported for poller
static uv_timer_t g_freeze_trigger_timer;
static uv_timer_t g_poller_timer;  // [Added] Poller Loop

static void _poller_timer_cb(uv_timer_t* handle) {
    (void)handle;
    dcu_poller_pump(&g_dcu_poller, &g_dcu_inventory);
}

typedef struct {
    datahub_t* hub;
    stream_engine_t* se;
} sync_ctx_t;

static sync_ctx_t g_sync_ctx;

// Linker Stubs
int northbound_mgr_start(void* mgr) {
    (void)mgr;
    return 0;
}
void northbound_mgr_stop(void* mgr) {
    (void)mgr;
}

static void on_sig_async(uv_signal_t* handle, int sig) {
    (void)handle;
    (void)sig;
    LOG_INFO(LOG_CAT_SCHED, "DCU: Received termination signal via libuv, flushing data...");
    dcu_storage_flush(&g_dcu_storage);
    if (g_running_svc) service_stop(g_running_svc);
}

static void on_safety_timeout(uv_timer_t* handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_SCHED, "🏁 [SAFETY] Timeout reached, initiating clean shutdown...");
    dcu_storage_flush(&g_dcu_storage);
    if (g_running_svc) service_stop(g_running_svc);
}

static int g_test_cycles = 0;

static void _freeze_trigger_cb(uv_timer_t* handle) {
    sync_ctx_t* ctx = (sync_ctx_t*)handle->data;
    pipe_msg_t msg;
    int saved = 0;

    for (int i = 0; i < g_dcu_inventory.count; i++) {
        int shadow_idx = dcu_tag_map_fn(0x20000200, (uint32_t)i);
        if (datahub_read_shadow_idx(ctx->hub, (uint32_t)shadow_idx, &msg) == 0) {
            dcu_storage_write_telemetry(&g_dcu_storage, g_dcu_inventory.meters[i].address,
                                        (uint32_t)i, 0x20000200, (float)msg.payload.val.f);
            saved++;
        }
    }
    if (saved > 0) {
        dcu_storage_flush(&g_dcu_storage);
        LOG_INFO(LOG_CAT_SCHED, "✅ [STORAGE] SUCCESS! Captured %d meters truth data.", saved);
        printf("💓 [HEARTBEAT] Marathon Cycle %d Completed. Database Flushed.\n",
               g_test_cycles + 1);

        // 💡 [Industrial Fix] 开启大事务保护 5xxx 冻结管线的高频批量写入，防止 SQLite 疯狂分配/释放
        // heap 导致 C 库崩溃
        dcu_storage_begin_trans(&g_dcu_storage);

        for (int i = 0; i < g_dcu_inventory.count; i++) {
            pipe_msg_t trigger;
            memset(&trigger, 0, sizeof(pipe_msg_t));
            trigger.tag_id = OAD_DAILY_FREEZE;
            trigger.type = PIPEMSG_TYPE_SCALAR;
            trigger.device_id = (uint32_t)i;
            trigger.ts = (uint64_t)time(NULL);
            if (ctx->se) stream_engine_push(ctx->se, &trigger);

            // 💡 [8xxx Protocol Verification] 随机触发跳闸控制，验证控制闭环
            if (i > 0 && i % 200 == 0) {
                uint32_t relay_omd = (0x8000U << 16) | 0x0100;
                uint8_t action =
                    (g_test_cycles % 2 == 0) ? 1 : 0;  // 0/2 cycle Trip, 1 cycle Restore
                printf("🎛️ [RELAY] Dev %d: Triggering Action %d (OI 8000 Attr 1)\n", i, action);
                dcu_router_action(relay_omd, (void*)(uintptr_t)i, &action);
            }
        }

        dcu_storage_commit_trans(&g_dcu_storage);
    }

    if (++g_test_cycles >= 3) {
        LOG_INFO(LOG_CAT_SCHED, "🏁 [TEST] Marathon Completed. Self-terminating...");
        on_safety_timeout(NULL);
    }
}

static uv_timer_t g_control_test_timer;

static void _control_test_cb(uv_timer_t* handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_BUS,
             "🏁 [TEST] Triggering Remote Control ACTION (8000) for Meter 10000001...");
    uint32_t omd = (0x8000U << 16) | 0x0100;
    dcu_router_action(omd, NULL, NULL);
}

// === [Consumer-Driven Interfaces] Wrappers ===
static int _poller_publish_realtime(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                    uint64_t timestamp_ns) {
    datahub_t* hub = (datahub_t*)ctx;
    if (!hub) return -1;
    pipe_msg_t bus_msg;
    pipe_msg_init_scalar(&bus_msg, tag_id, val, timestamp_ns);
    bus_msg.device_id = meter_idx;
    return datahub_publish(hub, &bus_msg);
}

static int _poller_push_event(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                              uint64_t timestamp_ns) {
    (void)ctx;
    extern void* g_running_se;
    if (!g_running_se) return -1;
    pipe_msg_t bus_msg;
    pipe_msg_init_scalar(&bus_msg, tag_id, val, timestamp_ns);
    bus_msg.device_id = meter_idx;
    stream_engine_push((stream_engine_t*)g_running_se, &bus_msg);
    return 0;
}

static int _event_archive_event(void* ctx, const char* addr, uint16_t event_code,
                                uint64_t start_time, uint64_t end_time, float snapshot_val) {
    return dcu_storage_write_event((dcu_storage_t*)ctx, addr, event_code, start_time, end_time,
                                   snapshot_val);
}

static int _event_archive_freeze(void* ctx, const char* addr, uint32_t freeze_oad,
                                 uint64_t freeze_time, uint32_t src_oad, double value) {
    return dcu_storage_write_freeze((dcu_storage_t*)ctx, addr, freeze_oad, freeze_time, src_oad,
                                    value);
}

static int _router_count_freezes(void* ctx, uint16_t* out_count) {
    return dcu_storage_count_freezes((dcu_storage_t*)ctx, out_count);
}

static int _router_read_shadow(void* ctx, uint32_t idx, void* out_msg) {
    return datahub_read_shadow_idx((datahub_t*)ctx, idx, (pipe_msg_t*)out_msg);
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    (void)argc;
    (void)argv;
    LOG_INFO(LOG_CAT_SCHED, "🚀 Starting EdgeC DCU (Series 6xxx) - Digital Twin Marathon");

    uv_loop_t* loop = uv_default_loop();
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);

    dcu_inventory_init(&g_dcu_inventory);

    const char* db_abs_path = "/workspaces/edgeos/EdgeC/build/products/dcu/bin/dcu_runtime.db";
    if (dcu_storage_init(&g_dcu_storage, loop, db_abs_path) != 0) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to initialize storage.");
        return -1;
    }

    int loaded = dcu_inventory_load_from_db(&g_dcu_inventory, db_abs_path);
    if (loaded < 0 || g_dcu_inventory.count == 0) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to load meters (Count: %d).",
                  g_dcu_inventory.count);
        return -2;
    }
    LOG_INFO(LOG_CAT_SCHED, "📖 [ARCHIVE] Loaded %d meters from Digital Twin database.",
             g_dcu_inventory.count);

    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 32768, 32768, NULL, 0, NULL, NULL, NULL);
    hub.map_fn_v2 = dcu_tag_map_fn;

    memset(&g_evt_3000, 0, sizeof(dcu_event_obj_t));
    g_evt_3000.oi = OI_VOLTAGE_DROP;
    g_evt_3000.threshold = 176.0f;
    g_evt_3000.delay_s = 0;
    g_evt_3000.is_enabled = true;
    g_evt_3000.snapshot_count = 2;
    g_evt_3000.snapshot_oads[0] = OAD_VOLTAGE;
    g_evt_3000.snapshot_oads[1] = OAD_ACTIVE_ENERGY;

    memset(&g_evt_3005, 0, sizeof(dcu_event_obj_t));
    g_evt_3005.oi = OI_OVER_CURRENT;
    g_evt_3005.threshold = 5.0f;
    g_evt_3005.delay_s = 0;
    g_evt_3005.is_enabled = true;
    g_evt_3005.snapshot_count = 1;
    g_evt_3005.snapshot_oads[0] = OAD_CURRENT;

    router_sink_ops_t router_sink = {.ctx = &g_dcu_storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _router_count_freezes};

    router_bus_ops_t router_bus = {.ctx = &hub,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _poller_publish_realtime,
                                   .read_shadow_idx = _router_read_shadow};

    dcu_router_init_ex(&g_dcu_inventory, &g_dcu_poller, router_sink, NULL, NULL, &g_evt_3000,
                       &g_evt_3005, router_bus);

    arbiter_t* arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    driver_manager_t* drv_mgr = driver_manager_create();
    edge_driver_registry_init(drv_mgr);

    bus_orchestrator_t* io_orch =
        bus_orchestrator_create(&hub, arbiter_ctx, drv_mgr, 64, 8192, 4096);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    arbiter_resume(arbiter_ctx);

    for (int i = 0; i < 32; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/tmp/rs485_%d.sock", i);
        const char* proto = (i < 22) ? "DLT698" : "DLT645";
        const driver_contract_injection_t* inj = contract_model_find(proto);
        if (inj) bus_orchestrator_setup_master(io_orch, i, proto, 0, inj);
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) >= 0) {
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t* h = io_executor_create_from_fd(loop, fd, hub.io_pool);
            bus_orchestrator_attach_io(io_orch, i, h);
        } else {
            close(fd);
        }
    }

    dcu_poller_init(&g_dcu_poller);
    uint32_t oads[1] = {0x20000200};
    dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 1);
    dcu_poller_add_task(&g_dcu_poller, 1, 5, 1);

    service_t* svc = service_new(loop, &hub, arbiter_ctx, NULL, NULL, NULL, NULL);
    g_running_svc = svc;

    // [Safe Signals]
    static uv_signal_t sig_term, sig_int;
    uv_signal_init(loop, &sig_term);
    uv_signal_start(&sig_term, on_sig_async, SIGTERM);
    uv_signal_init(loop, &sig_int);
    uv_signal_start(&sig_int, on_sig_async, SIGINT);

    // === [Consumer-Driven Interfaces] Injecting Dependencies into Poller ===
    poller_sink_ops_t poller_sink = {.ctx = &g_dcu_storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .commit_batch = (void*)dcu_storage_flush};
    poller_bus_ops_t poller_bus = {.ctx = &hub,
                                   .publish_realtime = _poller_publish_realtime,
                                   .push_event = _poller_push_event};

    dcu_poller_start(&g_dcu_poller,
                     (poller_io_ops_t){0},  // We skip IO ops for now since we are in bypass mode
                     poller_bus, poller_sink);

    stream_engine_t* se =
        stream_engine_create(&hub, loop, (struct bus_orchestrator_s*)io_orch, NULL);
    g_running_se = se;
    event_sink_ops_t event_sink = {.ctx = &g_dcu_storage,
                                   .archive_event = _event_archive_event,
                                   .archive_freeze = _event_archive_freeze};

    pipeline_t* evt_pipe = stream_engine_add_pipeline(se, "EVENT_PIPELINE", 0);
    pipeline_stage_t* m_stage = stage_event_monitor_create(&g_evt_3000, &g_evt_3005);
    pipeline_add_stage(evt_pipe, m_stage->name, m_stage->process, m_stage->priv_state,
                       m_stage->cleanup_state, NULL, NULL);
    free(m_stage);
    pipeline_stage_t* l_stage =
        stage_event_logger_create(&g_evt_3000, &g_evt_3005, &hub, event_sink);
    pipeline_add_stage(evt_pipe, l_stage->name, l_stage->process, l_stage->priv_state,
                       l_stage->cleanup_state, NULL, NULL);
    free(l_stage);

    pipeline_t* frz_pipe = stream_engine_add_pipeline(se, "FREEZE_PIPELINE", 0);
    pipeline_stage_t* f_stage = stage_periodic_freeze_create_ex(&hub, event_sink);
    pipeline_add_stage(frz_pipe, f_stage->name, f_stage->process, f_stage->priv_state,
                       f_stage->cleanup_state, NULL, NULL);
    free(f_stage);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = &hub;
    g_sync_ctx.se = se;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    // [Industrial Loop] 启动业务采集轮询定时器
    uv_timer_init(loop, &g_poller_timer);
    uv_timer_start(&g_poller_timer, _poller_timer_cb, 1000, 5000);

    uv_timer_init(loop, &g_control_test_timer);
    uv_timer_start(&g_control_test_timer, _control_test_cb, 30000, 0);

    static uv_timer_t safety_timer;
    uv_timer_init(loop, &safety_timer);
    uv_timer_start(&safety_timer, on_safety_timeout, 60000, 0);

    bus_orchestrator_run(io_orch);
    return service_run(svc);
}
