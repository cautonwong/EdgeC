/**
 * @file iems_app.c
 * @brief IEMS Application Assembler and Lifecycle Implementation (AEP-406 & AEP-400)
 */

#include "iems_app.h"

#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "datahub.h"
#include "edge_north_plane.h"
#include "edge_slave_plane.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_driver_runtime.h"
#include "edge_log.h"
#include "edgeos/edge_app_mgr.h"
#include "mqtt_json_codec.h"
#include "edge_service.h"
#include "edge_params_sqlite_adapter.h"
#include "edge_params.h"
#include "iems_app_manager_adapter.h"
#include "iems_app_runtime.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_internal_bus_adapter.h"
#include "iems_north_adapter.h"
#include "iems_north_listener.h"
#include "iems_poll_scheduler.h"
#include "iems_pressure_adapter.h"
#include "iems_southbound_adapter.h"
#include "iems_ui_channel.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_sqlite_adapter.h"
#include "io_executor.h"
#include "modbus_codec.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "north_driver_adapter.h"
#include "north_source_datahub_adapter.h"
#include "snf_sqlite_adapter.h"
#include "rest_server.h"
#include "rest_uds_server.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

static void _on_app_fault(void* ctx, const char* app_id, int64_t status, int term_signal) {
    iems_control_app_faulted((iems_control_ctx_t*)ctx, app_id, status, term_signal);
}

static void _on_app_restarted(void* ctx, const char* app_id) {
    iems_control_app_restarted((iems_control_ctx_t*)ctx, app_id);
}

struct iems_app_s {
    uv_loop_t* loop;
    iems_node_t* node;
    iems_node_storage_t node_storage;
    service_t* svc;
    datahub_t* hub;
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    edge_driver_runtime_t driver_runtime;
    snf_store_port_t snf_store;
    stream_engine_t* stream_eng;
    northbound_mgr_t* north;
    north_driver_adapter_storage_t north_driver_storage;
    iems_north_ctx_t north_adapter;
    sqlite_svc_t* db_service;
    char* db_path;
    edge_app_mgr_t* app_mgr;
    void* app_mgr_mem;
    edge_params_t* pipeline_params;
    edge_params_storage_t pipeline_params_storage;
    edge_params_store_t pipeline_param_store;
    iems_control_ctx_t control_ctx;
    rest_control_port_t control;

    iems_poll_scheduler_t poll;
    iems_internal_bus_t internal_bus;

    bool buses_attempted;
    bus_conn_state_t bus_state[IEMS_MAX_VBUS];

    bool monitor_initialized;
    uv_timer_t timer_monitor;

    bool pressure_initialized;
    app_pressure_t pressure;
    app_pressure_port_t pressure_port;
    iems_pressure_ctx_t pressure_ctx;
    uv_timer_t timer_pressure;

    bool reconcile_initialized;
    uv_timer_t timer_reconcile;
    uint64_t last_time_sync_ms;

    iems_ui_channel_t ui;
    iems_north_listener_t north_listener;
    rest_uds_server_t* ctl_server;
    iems_app_runtime_t* app_runtime;
};

static void _on_pressure_timer(uv_timer_t* timer) {
    iems_app_t* app = (iems_app_t*)timer->data;
    if (!app || !app->pressure_initialized) return;
    iems_pressure_tick(&app->pressure, uv_now(app->loop));
}

static int _reconcile_app_running(void* ctx, const char* app_id) {
    iems_app_t* app = (iems_app_t*)ctx;
    if (!app->app_runtime) return 1; /* without a runtime the registry is all we have */
    return iems_app_runtime_is_running(app->app_runtime, app_id);
}

static int _reconcile_app_start(void* ctx, const char* app_id) {
    iems_app_t* app = (iems_app_t*)ctx;
    return app->control.start_app ? app->control.start_app(app->control.ctx, app_id) : -1;
}

static int _reconcile_app_stop(void* ctx, const char* app_id) {
    iems_app_t* app = (iems_app_t*)ctx;
    return app->control.stop_app ? app->control.stop_app(app->control.ctx, app_id) : -1;
}

/* The OS time daemon reports a completed sync by writing the epoch to a small
 * file (EDGEC_TIME_SYNC_FILE, default /run/edgeos/time_sync). The product only
 * consumes it; it never talks to NTP itself. */
static void _poll_time_sync(iems_app_t* app) {
    if (!app || !app->control.time_mark) return;
    const char* path = getenv("EDGEC_TIME_SYNC_FILE");
    if (!path || !path[0]) path = "/run/edgeos/time_sync";
    FILE* f = fopen(path, "r");
    if (!f) return;
    unsigned long long value = 0;
    int fields = fscanf(f, "%llu", &value);
    fclose(f);
    if (fields != 1 || value == 0) return;
    uint64_t epoch_ms = value < 100000000000ull ? (uint64_t)value * 1000ull : (uint64_t)value;
    if (epoch_ms <= app->last_time_sync_ms) return;
    if (app->control.time_mark(app->control.ctx, epoch_ms, NULL, 0) == 0) {
        app->last_time_sync_ms = epoch_ms;
        LOG_INFO(LOG_CAT_CORE, "time: synced at %llu", (unsigned long long)epoch_ms);
    }
}

static void _on_reconcile_timer(uv_timer_t* timer) {
    iems_app_t* app = (iems_app_t*)timer->data;
    if (!app) return;
    _poll_time_sync(app);
    if (!app->app_mgr || edge_app_mgr_is_busy(app->app_mgr)) return;
    int acted = 0;
    if (edge_app_mgr_reconcile(app->app_mgr, _reconcile_app_running, _reconcile_app_start,
                               _reconcile_app_stop, app, &acted) == 0 &&
        acted > 0) {
        LOG_INFO(LOG_CAT_CORE, "reconcile: converged %d app(s)", acted);
    }
}

/**
 * @brief [Domain Boundary Guard] 控制写路径强制经聚合根 (Issue #400)
 */
static bool _iems_production_env(void) {
    const char *env = getenv("EDGE_ENV");
    return env && (strcmp(env, "production") == 0 || strcmp(env, "prod") == 0);
}

static int _iems_stream_submit_control(void* ctx, int bus_id, uint32_t tag_id, double value,
                                       const char* proto) {
    iems_app_t* app = (iems_app_t*)ctx;
    if (!app || !app->node) return -1;

    int rc = iems_node_write_tag(app->node, bus_id, tag_id, value, proto);
    if (rc < 0) {
        LOG_WARN(LOG_CAT_CORE, "IEMS control write rejected by aggregate root (rc=%d)", rc);
    }
    return rc;
}

int iems_app_connect_buses(iems_app_t* app, int* configured) {
    if (configured) *configured = 0;
    if (!app) return -1;
    int rc =
        iems_southbound_connect(app->loop, app->hub, app->orch, app->bus_state, configured);
    app->buses_attempted = (rc >= 0);  // 只在装配契约可用时才算「连过了」
    return rc;
}

static void _monitor_shadow_tick(uv_timer_t* handle) {
    iems_app_t* app = (iems_app_t*)handle->data;
    if (app && app->hub && datahub_shadow_ready(app->hub)) {
        pipe_msg_t msg;
        if (datahub_read_shadow(app->hub, IEMS_TAG_VOLTAGE_PHASE_A, &msg) == 0) {
            printf("\033[36m[HEARTBEAT] Shadow Tag 1001 = %.2f\033[0m\n", msg.payload.val.f);
        }
    }
}

static const edge_param_info_t k_iems_pipeline_params[] = {
    {.name = "batt_max_soc", .type = PARAM_TYPE_FLOAT, .def_val.f = 95.0, .id = 1,
     .unit = "%", .min = 0.0, .max = 100.0, .desc = "Battery max SOC"},
    {.name = "batt_min_soc", .type = PARAM_TYPE_FLOAT, .def_val.f = 10.0, .id = 2,
     .unit = "%", .min = 0.0, .max = 100.0, .desc = "Battery min SOC"},
    {.name = "batt_max_charge", .type = PARAM_TYPE_FLOAT, .def_val.f = 5000.0, .id = 3,
     .unit = "W", .min = 0.0, .max = 1e9, .desc = "Battery max charge power"},
    {.name = "batt_max_discharge", .type = PARAM_TYPE_FLOAT, .def_val.f = 5000.0, .id = 4,
     .unit = "W", .min = 0.0, .max = 1e9, .desc = "Battery max discharge power"},
};

static double _iems_pipeline_config_double(void *ctx, const char *key, double def_val) {
    return edge_param_get_float((edge_params_t *)ctx, key, def_val);
}

static pipeline_loader_ctx_t _iems_loader_ctx(iems_app_t* app) {
    pipeline_loader_ctx_t ctx = {
        .db = stream_sqlite_port_from_sqlite(app->db_service),
        .control_ops = {.ctx = app, .submit_control = _iems_stream_submit_control},
        .config = {.ctx = app->pipeline_params,
                   .get_double = _iems_pipeline_config_double},
        .audit_secret = getenv("VPP_AUDIT_SECRET"),
        .production = _iems_production_env()
    };
    return ctx;
}

static void _reactivate_desired_apps(iems_app_t* app);

static void _start_app_management(iems_app_t* app) {
    _poll_time_sync(app);
    int recovered = edge_app_mgr_recover_packages(app->app_mgr);
    if (recovered > 0) {
        LOG_WARN(LOG_CAT_CORE, "reconcile: recovered %d package(s) from interrupted upgrades",
                 recovered);
    }
    _reactivate_desired_apps(app);
}

/* desired_state survives a host restart; bring those apps back without a manual start. */
static void _reactivate_desired_apps(iems_app_t* app) {
    if (!app || !app->app_mgr || !app->control.start_app) return;
    const edge_app_repo_t* repo = edge_app_mgr_get_repo(app->app_mgr);
    if (!repo || !repo->list_apps) return;
    app_manifest_t apps[64];
    int count = repo->list_apps(repo->ctx, apps, 64);
    for (int i = 0; i < count; i++) {
        if (!apps[i].desired_state) continue;
        if (apps[i].state == APP_STATE_UNINSTALLED || apps[i].state == APP_STATE_FAULTED) continue;
        app->control.start_app(app->control.ctx, apps[i].id);
    }
}

iems_app_t* iems_app_assemble(uv_loop_t* loop, const iems_app_config_t* cfg) {
    if (!loop) return NULL;

    iems_app_t* app = (iems_app_t*)calloc(1, sizeof(iems_app_t));
    if (!app) return NULL;

    app->loop = loop;
    loop->data = app;

    // =========================================================================
    // Phase 1: 实例化 IEMS 聚合根 (Aggregate Root)
    // =========================================================================
    iems_node_config_t node_cfg = {
        .product_name = "IEMS",
        .shm_name = (cfg && cfg->shm_name) ? cfg->shm_name : EDGE_SHM_BUS_NAME,
        .datahub_capacity = 1024,
        .max_pcs_power_kw = 500.0,
        .transformer_capacity_kva = 630.0,
        .demand_limit_kw = 500.0
    };
    app->node = iems_node_create_in(&app->node_storage, loop, &node_cfg);
    if (!app->node) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS Assembler: Failed to create iems_node aggregate root");
        free(app);
        return NULL;
    }
    app->hub = iems_node_get_datahub(app->node);
    app->north_adapter.node = app->node;
    app->north_adapter.hub = app->hub;
    app->north_adapter.submit_control = _iems_stream_submit_control;
    app->north_adapter.submit_ctx = app;
    iems_node_set_execution_port(app->node, iems_execution_port_create(app));

    // =========================================================================
    // Phase 2: DataHub 高可靠服务 (Metrics, WAL, Snapshots, Compaction)
    // =========================================================================
    datahub_start_metrics(app->hub);
    const char* wal_path = (cfg && cfg->wal_path) ? cfg->wal_path : "/tmp/edgeos_wal.log";
    const char* snap_path = (cfg && cfg->snap_path) ? cfg->snap_path : "/tmp/edgeos_snapshot.bin";
    datahub_load_snapshot(app->hub, snap_path);
    datahub_enable_wal(app->hub, wal_path);
    datahub_replay_wal(app->hub);
    datahub_start_compaction(app->hub, 300000, snap_path);

    // =========================================================================
    // Phase 3: 南向调度总线与驱动管理器 (Bus Orchestrator & Driver Manager)
    // =========================================================================
    arbiter_init(&app->arb, loop);
    if (edge_driver_runtime_init(&app->driver_runtime) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to create driver runtime");
        return NULL;
    }
    iems_driver_registry_init(&app->driver_runtime.registry);

    app->orch = bus_orchestrator_create_hub_default(app->hub, app->arb, &app->driver_runtime.session, 256,
                                        256, 2048);
    datahub_bind_io_pool(app->hub, bus_orchestrator_get_pool(app->orch));
    edge_api_init(app->orch);

    uint32_t conn_count = 0;
    const edge_connectivity_t* conns = iems_get_connectivity(&conn_count);
    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* conn = &conns[i];
        if (strcmp(conn->category, "SOUTHBOUND") == 0) {
            bus_orchestrator_setup_master(app->orch, conn->id, "ModbusRTU", (uint32_t)conn->id);
        } else if (strcmp(conn->category, "NORTHBOUND") == 0) {
            bus_orchestrator_setup_slave(app->orch, conn->id, "ModbusRTU", 1);
            /* 从站应答的"当前值"由产品给：驱动不认识点位模型 */
            bus_orchestrator_set_slave_source(app->orch, conn->id, iems_north_slave_ops(),
                                              &app->north_adapter);
        }
    }

    // =========================================================================
    // Phase 4: 本地持久化与流计算引擎 (SQLite & StreamEngine)
    // =========================================================================
    const char* db_file = (cfg && cfg->db_path) ? cfg->db_path : "iems.db";
    app->db_path = strdup(db_file);
    sqlite_config_t db_conf = {.db_path = app->db_path, .strategy = STRATEGY_LITE};
    app->db_service = sqlite_svc_create(loop, &db_conf);
    if (app->db_service) {
        app->pipeline_param_store = edge_params_sqlite_store(app->db_service);
        app->pipeline_params = edge_params_create_in(&app->pipeline_params_storage, &app->pipeline_param_store, NULL);
        if (app->pipeline_params) {
            edge_params_register_metadata(app->pipeline_params, k_iems_pipeline_params,
                                          (int)(sizeof(k_iems_pipeline_params) /
                                                sizeof(k_iems_pipeline_params[0])));
        }
    }
    iems_poll_scheduler_init(&app->poll, loop, app->node, app->db_service);

    app->stream_eng =
        stream_engine_create_datahub(app->hub, NULL, app->orch, edge_stage_factory_get_registry());
    pipeline_loader_ctx_t loader_ctx = _iems_loader_ctx(app);
    uint32_t pipeline_count = 0;
    const edge_pipeline_def_t* pipelines = iems_get_pipelines(&pipeline_count);
    stream_engine_load_static_metadata(app->stream_eng, pipelines, (int)pipeline_count, &loader_ctx);
    stream_engine_start(app->stream_eng);

    // =========================================================================
    // Phase 5: 微运行时与北向通信 (WASM & Northbound MQTT)
    // =========================================================================
    stream_bus_port_t wasm_bus = stream_bus_datahub_port(app->hub);
    wasm_mgr_init(&wasm_bus);

    app->north = (northbound_mgr_t*)calloc(1, sizeof(northbound_mgr_t));
    const char* mqtt_url = (cfg && cfg->mqtt_url) ? cfg->mqtt_url : "tcp://localhost:1883";
    nb_transport_ops_t transport = mqtt_producer_create(mqtt_url, "IEMS_EDGE_NODE");
    nb_source_ops_t nb_source = northbound_source_from_datahub(app->hub);
    if (app->db_service) app->snf_store = snf_sqlite_store_port(app->db_service);
    northbound_mgr_init_ex(app->north, nb_source, "IEMS_V4",
                           app->db_service ? &app->snf_store : NULL, transport);
    northbound_mgr_set_driver_port(app->north,
                                   north_driver_adapter_init(&app->north_driver_storage));
    /* 主题身份在这里读一次 env 并交给北向管理器，深层模块不再自己 getenv。 */
    northbound_mgr_set_address(app->north, NULL, getenv("MQTT_GROUP_ID"),
                               getenv("MQTT_NODE_ID"));
    /* 北向取数端口：metric 名与当前值由产品给，驱动不反查 datahub。 */
    app->north_adapter.north = app->north;
    northbound_mgr_set_north_source(app->north, iems_north_source_ops(),
                                    &app->north_adapter);
    /* 协议可由部署选（E2E 用 MqttJson：载荷是 JSON，平台侧能解；
     * 现场默认仍是 Sparkplug B）。换协议时地址模板跟着换。 */
    const char* nb_proto = getenv("MQTT_PROTOCOL");
    if (nb_proto && *nb_proto && northbound_mgr_set_protocol(app->north, nb_proto) != 0) {
        LOG_WARN(LOG_CAT_NB, "Northbound: unknown protocol '%s', keeping default", nb_proto);
    }

    // =========================================================================
    // Phase 6: 系统服务、REST API 与 UI 伴随通道
    // =========================================================================
    app->svc = service_new(loop, app->hub, app->arb, app->stream_eng, NULL);
    app->app_mgr = iems_app_manager_create(loop, app->db_service, app->stream_eng,
                                           &app->app_mgr_mem);
    iems_control_port_init(&app->control, &app->control_ctx, app->app_mgr,
                           app->stream_eng, app->db_service);
    service_set_control_port(app->svc, &app->control);

    uint32_t app_tag_count = 0;
    const edge_tag_meta_t* app_tags = iems_get_tag_metadata(&app_tag_count);
    iems_app_runtime_config_t runtime_cfg = {
        .hub = app->hub,
        .db = app->db_service,
        .tags = app_tags,
        .tag_count = app_tag_count,
        .app_base = 131072u,
        .on_fault = _on_app_fault,
        .on_fault_ctx = &app->control_ctx,
        .on_restarted = _on_app_restarted,
        .on_restarted_ctx = &app->control_ctx,
    };
    app->app_runtime = iems_app_runtime_create(loop, &runtime_cfg);
    if (!app->app_runtime) {
        LOG_WARN(LOG_CAT_CORE, "AppHost: runtime unavailable; apps fall back to the in-process stage");
    }
    iems_control_set_app_runtime(&app->control_ctx, app->app_runtime);

    iems_pressure_init(&app->pressure, &app->pressure_port, &app->pressure_ctx, app->app_mgr,
                       app->app_runtime, &app->control);
    app->pressure_initialized = true;
    uv_timer_init(loop, &app->timer_pressure);
    app->timer_pressure.data = app;
    uv_timer_start(&app->timer_pressure, _on_pressure_timer, 3000, 3000);

    uv_timer_init(loop, &app->timer_reconcile);
    app->timer_reconcile.data = app;
    uint64_t reconcile_ms = 30000;
    const char* reconcile_env = getenv("EDGEC_RECONCILE_INTERVAL_MS");
    if (reconcile_env && reconcile_env[0]) {
        unsigned long value = strtoul(reconcile_env, NULL, 10);
        if (value >= 100) reconcile_ms = value;
    }
    uv_timer_start(&app->timer_reconcile, _on_reconcile_timer, reconcile_ms, reconcile_ms);
    app->reconcile_initialized = true;

    int rest_port = (cfg && cfg->rest_port > 0) ? cfg->rest_port : 9091;
    rest_limits_t rest_limits;
    rest_limits_default(&rest_limits);
    if (cfg && cfg->rest_bind) rest_limits.bind_addr = cfg->rest_bind;
    /* 旧 raw wasm 上传入口默认关闭；只有显式配置才恢复，供迁移期使用。 */
    rest_server_set_legacy_upload(cfg && cfg->rest_legacy_upload);
    rest_server_create_in_ex(NULL, loop, app->svc, rest_port, &rest_limits);

    const char* ctl_sock = getenv("EDGEC_APPCTL_SOCK");
    if (!ctl_sock || !ctl_sock[0]) ctl_sock = "/tmp/edgeos_appctl.sock";
    app->ctl_server = rest_uds_server_start(loop, app->svc, ctl_sock);
    if (!app->ctl_server) {
        LOG_WARN(LOG_CAT_CORE, "appctl: management socket unavailable at %s", ctl_sock);
    }

    const char* ui_path =
        (cfg && cfg->ui_sock && cfg->ui_sock[0]) ? cfg->ui_sock : EDGE_UI_SOCKET_PATH;
    iems_ui_channel_start(&app->ui, loop, app->hub, ui_path);

    iems_north_listener_start(&app->north_listener, loop, app->hub, app->orch,
                              (cfg ? cfg->north_sock : NULL), app->bus_state);

    _start_app_management(app);
    return app;
}

int iems_app_start(iems_app_t* app) {
    if (!app) return -1;

    // 南向总线只由组合根连一次;没走过那一步就启动 = 半装配,拒绝起
    if (!app->buses_attempted) {
        LOG_ERROR(LOG_CAT_CORE,
                  "IEMS start: iems_app_connect_buses() was never called; southbound buses are "
                  "not wired. Boot aborted.");
        return -1;
    }

    iems_node_start(app->node);

    iems_poll_scheduler_start(&app->poll);

    uv_timer_init(app->loop, &app->timer_monitor);
    app->timer_monitor.data = app;
    app->monitor_initialized = true;  // 此刻起 destroy 必须关掉这个已 uv_init 的 timer
    uv_timer_start(&app->timer_monitor, _monitor_shadow_tick, 1000, 1000);

    iems_internal_bus_start(&app->internal_bus, app->loop, app->hub);

    arbiter_resume(app->arb);
    LOG_INFO(LOG_CAT_CORE, "IEMS Application started successfully");
    return 0;
}

int iems_app_run(iems_app_t* app) {
    if (!app || !app->svc) return -1;
    if (app->north) northbound_mgr_start(app->north);
    return service_run(app->svc);
}

void iems_app_destroy(iems_app_t* app) {
    if (!app) return;

    iems_poll_scheduler_stop(&app->poll);

    // timer_monitor 的 uv_init 在 iems_app_start 里;
    // 启动前就调用 destroy 时(组合根在 connect 失败后退场)不能 uv_close 未初始化的 handle。
    if (app->monitor_initialized) {
        uv_timer_stop(&app->timer_monitor);
        uv_close((uv_handle_t*)&app->timer_monitor, NULL);
    }
    if (app->pressure_initialized) {
        uv_timer_stop(&app->timer_pressure);
        uv_close((uv_handle_t*)&app->timer_pressure, NULL);
    }
    if (app->reconcile_initialized) {
        uv_timer_stop(&app->timer_reconcile);
        uv_close((uv_handle_t*)&app->timer_reconcile, NULL);
    }
    iems_internal_bus_stop(&app->internal_bus);

    iems_ui_channel_close(&app->ui);
    iems_north_listener_close(&app->north_listener);
    rest_uds_server_stop(app->ctl_server);
    app->ctl_server = NULL;

    uv_run(app->loop, UV_RUN_NOWAIT);

    /* 异步 app 任务完成回调还会用 app_mgr 与 stream_engine，先排空再拆。 */
    while (edge_app_mgr_is_busy(app->app_mgr)) uv_run(app->loop, UV_RUN_ONCE);

    iems_app_runtime_destroy(app->app_runtime);
    app->app_runtime = NULL;

    iems_internal_bus_join(&app->internal_bus);

    if (app->stream_eng) stream_engine_destroy(app->stream_eng);
    wasm_mgr_cleanup();
    iems_app_manager_destroy(app->app_mgr, app->app_mgr_mem);
    if (app->north) northbound_mgr_stop(app->north);
    if (app->svc) service_delete(app->svc);
    if (app->pipeline_params) edge_params_destroy(app->pipeline_params);
    if (app->db_service) sqlite_svc_destroy(app->db_service);
    if (app->orch) bus_orchestrator_destroy(app->orch);
    if (app->arb) arbiter_destroy(app->arb);
    edge_driver_runtime_destroy(&app->driver_runtime);
    if (app->north) free(app->north);
    if (app->db_path) free(app->db_path);

    if (app->node) {
        iems_node_destroy(app->node);
        app->node = NULL;
        app->hub = NULL;
    }

    free(app);
}

iems_node_t* iems_app_get_node(iems_app_t* app) {
    return app ? app->node : NULL;
}
