#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "dcu_bus_adapter.h"
#include "dcu_driver_adapter.h"
#include "datahub.h"
#include "dcu_bus_source.h"
#include "dcu_heartbeat.h"
#include "dcu_inventory.h"
#include "dcu_node.h"
#include "dcu_north_adapter.h"
#include "dcu_north_source.h"
#include "dcu_pipeline_adapter.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "edge_api.h"
#include "dcu_twin_source.h"
#include "edge_driver_runtime.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "edge_slave_plane.h"
#include "edge_service.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "stream_bus_datahub_adapter.h"

// --- Global Contexts ---
static dcu_node_t* g_dcu_node = NULL;
static void *g_dcu_node_mem = NULL;

static service_t* g_running_svc = NULL;
void* g_running_se = NULL;  // Exported for poller

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

static int _create_dcu_node(uv_loop_t *loop, const dcu_node_config_t *cfg) {
    size_t storage_size = dcu_node_storage_size(cfg);
    g_dcu_node_mem = malloc(storage_size);
    if (!g_dcu_node_mem) return -1;
    g_dcu_node = dcu_node_create_in(g_dcu_node_mem, storage_size, loop, cfg);
    if (!g_dcu_node) {
        free(g_dcu_node_mem);
        g_dcu_node_mem = NULL;
        return -1;
    }
    return 0;
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
    static dcu_bus_source_t* bus_source = NULL;
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
    if (_create_dcu_node(loop, &node_cfg) != 0) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to initialize DCU node.");
        return -1;
    }

    dcu_inventory_t* inv = dcu_node_get_inventory(g_dcu_node);
    if (!inv || inv->count == 0) {
        LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: Failed to load meters (Count: %d).",
                  inv ? inv->count : 0);
        dcu_node_destroy(g_dcu_node);

        free(g_dcu_node_mem);

        g_dcu_node_mem = NULL;
        return -2;
    }
    LOG_INFO(LOG_CAT_SCHED, "📖 [ARCHIVE] Loaded %d meters from Digital Twin database.", inv->count);

    /* 表档案必须在第一帧之前注入驱动：之前只在 freeze 定时器里注入，
     * 前 5 秒发出的帧因此全部寻址到广播地址 0x99×6（多表串值的直接原因）。 */
    dcu_driver_adapter_set_meters(inv->meters, inv->count);

    datahub_t* hub = dcu_node_get_datahub(g_dcu_node);
    if (!bus_source && src_ops.submit == NULL) {
        // 组合根：依赖一次拿齐（hub + inv 已在手），不再有"先造后 bind"。
        // 显式配置的数据源（合成源）优先：它存在时总线源根本不该被创建。
        bus_source = dcu_bus_source_start(hub, inv, g_dcu_node, 4000);
        if (!bus_source) {
            LOG_ERROR(LOG_CAT_SCHED, "❌ FATAL: bus source alloc failed");
            dcu_node_destroy(g_dcu_node);

            free(g_dcu_node_mem);

            g_dcu_node_mem = NULL;
            return -1;
        }
        dcu_node_set_source_ops(g_dcu_node, dcu_bus_source_ops(bus_source));
    }
    dcu_storage_t* storage = dcu_node_get_storage(g_dcu_node);

    arbiter_t* arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    edge_driver_runtime_t driver_runtime;
    if (edge_driver_runtime_init(&driver_runtime) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to create driver runtime");
        return -1;
    }
    dcu_driver_registry_init(&driver_runtime.registry);

    bus_orchestrator_t* io_orch =
        bus_orchestrator_create_hub_default(hub, arbiter_ctx, &driver_runtime.session, 64, 8192, 4096);
    datahub_bind_io_pool(hub, bus_orchestrator_get_pool(io_orch));
    // 装配必须做全：把 orchestrator/驱动管理器登记到对外门面，否则 edge_api_read_tag 恒返回 -1
    edge_api_init(io_orch);
    arbiter_resume(arbiter_ctx);

    dcu_bus_attach_channels(loop, hub, io_orch, driver_runtime.manager, inv);

    dcu_north_slave_attach(loop, hub, io_orch);
    dcu_north_cloud_attach(hub);

    service_t* svc = service_new(loop, hub, arbiter_ctx, NULL, NULL);
    g_running_svc = svc;

    // [Safe Signals]
    static uv_signal_t sig_term, sig_int;
    uv_signal_init(loop, &sig_term);
    uv_signal_start(&sig_term, on_sig_async, SIGTERM);
    uv_signal_init(loop, &sig_int);
    uv_signal_start(&sig_int, on_sig_async, SIGINT);

    stream_engine_t* se =
        stream_engine_create_datahub(hub, loop, (struct bus_orchestrator_s*)io_orch, NULL);
    g_running_se = se;
    dcu_pipelines_attach(se, hub, inv, g_dcu_node, storage);

    // [Industrial Loop] 启动业务采集轮询
    dcu_node_start(g_dcu_node);

    dcu_heartbeat_t heartbeat;
    dcu_heartbeat_start(&heartbeat, loop, g_dcu_node, se, svc);

    static uv_timer_t safety_timer;
    uv_timer_init(loop, &safety_timer);
    uv_timer_start(&safety_timer, on_safety_timeout, 60000, 0);

    bus_orchestrator_run(io_orch);
    {
        northbound_mgr_t* north = dcu_north_mgr();
        if (north) northbound_mgr_start(north);
    }
    int ret = service_run(svc);
    // 先停北向线程再拆它读的 datahub，否则 uv 线程会读已释放的 hub。
    dcu_north_stop();
    dcu_bus_source_stop(bus_source); // 先停源再拆聚合根：源持有 hub/inv 指针
    bus_source = NULL;
    dcu_node_destroy(g_dcu_node);

    free(g_dcu_node_mem);

    g_dcu_node_mem = NULL;
    dcu_north_destroy();
    edge_driver_runtime_destroy(&driver_runtime);
    return ret;
}
