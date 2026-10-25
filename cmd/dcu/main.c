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
#include "dcu_driver_adapter.h"
#include "datahub.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_node.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "dcu_twin_source.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "edge_service.h"
#include "io_executor.h"
#include "sqlite_service.h"
#include "stream_engine.h"

// --- Global Contexts ---
static dcu_node_t* g_dcu_node = NULL;

static service_t* g_running_svc = NULL;
void* g_running_se = NULL;  // Exported for poller
static uv_timer_t g_freeze_trigger_timer;

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
    if (g_dcu_node) {
        dcu_storage_flush(dcu_node_get_storage(g_dcu_node));
    }
    if (g_running_svc) service_stop(g_running_svc);
}

static void on_safety_timeout(uv_timer_t* handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_SCHED, "🏁 [SAFETY] Timeout reached, initiating clean shutdown...");
    if (g_dcu_node) {
        dcu_storage_flush(dcu_node_get_storage(g_dcu_node));
    }
    if (g_running_svc) service_stop(g_running_svc);
}

static int g_test_cycles = 0;

static void _freeze_trigger_cb(uv_timer_t* handle) {
    sync_ctx_t* ctx = (sync_ctx_t*)handle->data;
    pipe_msg_t msg;
    int saved = 0;
    dcu_inventory_t* inv = dcu_node_get_inventory(g_dcu_node);
    dcu_storage_t* storage = dcu_node_get_storage(g_dcu_node);
    if (!inv || !storage) return;

    for (int i = 0; i < inv->count; i++) {
        if (datahub_read_shadow_device(ctx->hub, 0x20000200, (uint32_t)i, &msg) == 0) {
            dcu_storage_write_telemetry(storage, inv->meters[i].address, (uint32_t)i, 0x20000200,
                                        (float)msg.payload.val.f);
            saved++;
        }
    }
    if (saved > 0) {
        dcu_storage_flush(storage);
        LOG_INFO(LOG_CAT_SCHED, "✅ [STORAGE] SUCCESS! Captured %d meters truth data.", saved);
        printf("💓 [HEARTBEAT] Marathon Cycle %d Completed. Database Flushed.\n", g_test_cycles + 1);

        // 💡 [Industrial Fix] 开启大事务保护 5xxx 冻结管线的高频批量写入，防止 SQLite 疯狂分配/释放
        // heap 导致 C 库崩溃
        dcu_storage_begin_trans(storage);

        for (int i = 0; i < inv->count; i++) {
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
                printf("🎛️ [RELAY] Dev %d: Triggering Action %d (OI 8000 Attr 1)\n", i,
                       action);
                dcu_router_action(relay_omd, (void*)(uintptr_t)i, &action);
            }
        }

        dcu_storage_commit_trans(storage);
    }

    if (++g_test_cycles >= 3) {
        LOG_INFO(LOG_CAT_SCHED, "🏁 [TEST] Marathon Completed. Self-terminating...");
        on_safety_timeout(NULL);
    }
}

static uv_timer_t g_control_test_timer;

static void _control_test_cb(uv_timer_t* handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_BUS, "🏁 [TEST] Triggering Remote Control ACTION (8000) for Meter 10000001...");
    uint32_t omd = (0x8000U << 16) | 0x0100;
    dcu_router_action(omd, NULL, NULL);
}

// === [Consumer-Driven Interfaces] Wrappers ===
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

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    (void)argc;
    (void)argv;
    LOG_INFO(LOG_CAT_SCHED, "🚀 Starting EdgeC DCU (Series 6xxx) - Digital Twin Marathon");

    uv_loop_t* loop = uv_default_loop();

    const char* env_db = getenv("EDGE_DB_PATH");
    const char* env_data_dir = getenv("EDGEC_DATA_DIR");
    char db_resolved_path[512];
    if (env_db && strlen(env_db) > 0) {
        strncpy(db_resolved_path, env_db, sizeof(db_resolved_path) - 1);
        db_resolved_path[sizeof(db_resolved_path) - 1] = '\0';
    } else if (env_data_dir && strlen(env_data_dir) > 0) {
        snprintf(db_resolved_path, sizeof(db_resolved_path), "%s/dcu_runtime.db", env_data_dir);
    } else if (access("build/products/dcu/bin/dcu_runtime.db", F_OK) == 0) {
        strncpy(db_resolved_path, "build/products/dcu/bin/dcu_runtime.db",
                sizeof(db_resolved_path) - 1);
        db_resolved_path[sizeof(db_resolved_path) - 1] = '\0';
    } else {
        strncpy(db_resolved_path, "dcu_runtime.db", sizeof(db_resolved_path) - 1);
        db_resolved_path[sizeof(db_resolved_path) - 1] = '\0';
    }

    const char* db_abs_path = db_resolved_path;
    // 合成数据源仅供孪生演示；默认不接线，让 poller 走真实总线。
    poller_source_ops_t src_ops = {0};
    const char* env_twin = getenv("EDGE_DCU_TWIN_SOURCE");
    if (env_twin && strcmp(env_twin, "1") == 0) {
        static dcu_twin_source_t g_twin_source;
        dcu_twin_source_init(&g_twin_source, 220.0, 5.0);
        src_ops = dcu_twin_source_get_ops(&g_twin_source);
        LOG_WARN(LOG_CAT_SCHED,
                 "⚠️ EDGE_DCU_TWIN_SOURCE=1: poller uses synthetic values, bus reads are bypassed");
    }
    dcu_node_config_t node_cfg = {.product_name = "DCU",
                                  .db_path = db_abs_path,
                                  .datahub_capacity = 32768,
                                  .poll_interval_ms = 5000,
                                  .source_ops = src_ops};
    g_dcu_node = dcu_node_create(loop, &node_cfg);
    if (!g_dcu_node) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to initialize DCU node.");
        return -1;
    }

    dcu_inventory_t* inv = dcu_node_get_inventory(g_dcu_node);
    if (!inv || inv->count == 0) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to load meters (Count: %d).",
                  inv ? inv->count : 0);
        dcu_node_destroy(g_dcu_node);
        return -2;
    }
    LOG_INFO(LOG_CAT_SCHED, "📖 [ARCHIVE] Loaded %d meters from Digital Twin database.", inv->count);

    datahub_t* hub = dcu_node_get_datahub(g_dcu_node);
    dcu_storage_t* storage = dcu_node_get_storage(g_dcu_node);

    arbiter_t* arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    driver_manager_t* drv_mgr = driver_manager_create();
    dcu_driver_registry_init(drv_mgr);

    bus_orchestrator_t* io_orch =
        bus_orchestrator_create(hub, arbiter_ctx, drv_mgr, 64, 8192, 4096);
    datahub_bind_io_pool(hub, bus_orchestrator_get_pool(io_orch));
    arbiter_resume(arbiter_ctx);

    for (int i = 0; i < 32; i++) {
        // 协议来自档案，不按通道号猜；空通道不建主站、不连 socket。
        const char* proto = dcu_inventory_channel_protocol(inv, (uint8_t)i);
        if (!proto) continue;
        const driver_contract_injection_t* inj = dcu_get_driver_injection(proto);
        if (!inj) {
            LOG_ERROR(LOG_CAT_SCHED, "❌ channel %d: protocol '%s' has no registered driver", i,
                      proto);
            continue;
        }
        bus_orchestrator_setup_master(io_orch, i, proto, 0, inj);
        char path[64];
        snprintf(path, sizeof(path), "/tmp/rs485_%d.sock", i);
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) >= 0) {
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t* h = io_executor_create_from_fd(loop, fd, datahub_get_io_pool(hub));
            bus_orchestrator_attach_io(io_orch, i, h);
        } else {
            close(fd);
        }
    }

    service_t* svc = service_new(loop, hub, arbiter_ctx, NULL, NULL, NULL, NULL);
    g_running_svc = svc;

    // [Safe Signals]
    static uv_signal_t sig_term, sig_int;
    uv_signal_init(loop, &sig_term);
    uv_signal_start(&sig_term, on_sig_async, SIGTERM);
    uv_signal_init(loop, &sig_int);
    uv_signal_start(&sig_int, on_sig_async, SIGINT);

    stream_engine_t* se =
        stream_engine_create(hub, loop, (struct bus_orchestrator_s*)io_orch, NULL);
    g_running_se = se;
    event_sink_ops_t event_sink = {.ctx = storage,
                                   .archive_event = _event_archive_event,
                                   .archive_freeze = _event_archive_freeze};

    pipeline_t* evt_pipe = stream_engine_add_pipeline(se, "EVENT_PIPELINE", 0);
    pipeline_stage_t* m_stage = stage_event_monitor_create(inv, dcu_node_get_event_3000(g_dcu_node),
                                                           dcu_node_get_event_3005(g_dcu_node));
    pipeline_add_stage(evt_pipe, m_stage->name, m_stage->process, m_stage->priv_state,
                       m_stage->cleanup_state, NULL, NULL);
    free(m_stage);
    pipeline_stage_t* l_stage = stage_event_logger_create(
        inv, dcu_node_get_event_3000(g_dcu_node), dcu_node_get_event_3005(g_dcu_node), hub, event_sink);
    pipeline_add_stage(evt_pipe, l_stage->name, l_stage->process, l_stage->priv_state,
                       l_stage->cleanup_state, NULL, NULL);
    free(l_stage);

    pipeline_t* frz_pipe = stream_engine_add_pipeline(se, "FREEZE_PIPELINE", 0);
    pipeline_stage_t* f_stage = stage_periodic_freeze_create_ex(inv, hub, event_sink);
    pipeline_add_stage(frz_pipe, f_stage->name, f_stage->process, f_stage->priv_state,
                       f_stage->cleanup_state, NULL, NULL);
    free(f_stage);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = hub;
    g_sync_ctx.se = se;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    // [Industrial Loop] 启动业务采集轮询
    dcu_node_start(g_dcu_node);

    uv_timer_init(loop, &g_control_test_timer);
    uv_timer_start(&g_control_test_timer, _control_test_cb, 30000, 0);

    static uv_timer_t safety_timer;
    uv_timer_init(loop, &safety_timer);
    uv_timer_start(&safety_timer, on_safety_timeout, 60000, 0);

    bus_orchestrator_run(io_orch);
    int ret = service_run(svc);
    dcu_node_destroy(g_dcu_node);
    return ret;
}
