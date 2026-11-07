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
#include "datahub.h"
#include "edge_north_plane.h"
#include "edge_slave_plane.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_log.h"
#include "mqtt_json_codec.h"
#include "edge_service.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_internal_bus_adapter.h"
#include "iems_north_adapter.h"
#include "iems_poll_scheduler.h"
#include "iems_southbound_adapter.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_sqlite_adapter.h"
#include "io_executor.h"
#include "modbus_codec.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "rest_server.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

typedef struct {
    int bus_id;
    int bus_idx;
} north_accept_ctx_t;

struct iems_app_s {
    uv_loop_t* loop;
    iems_node_t* node;
    iems_node_storage_t node_storage;
    service_t* svc;
    datahub_t* hub;
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    driver_manager_t* drv_mgr;
    stream_engine_t* stream_eng;
    northbound_mgr_t* north;
    iems_north_ctx_t north_adapter;
    sqlite_svc_t* db_service;
    char* db_path;

    iems_poll_scheduler_t poll;
    iems_internal_bus_t internal_bus;

    bool buses_attempted;
    bus_conn_state_t bus_state[IEMS_MAX_VBUS];

    bool monitor_initialized;
    uv_timer_t timer_monitor;

    uv_pipe_t* ui_srv;
    uv_pipe_t* north_srv[IEMS_MAX_VBUS];
    north_accept_ctx_t north_ctx[IEMS_MAX_VBUS];
};

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

static void on_ui_connect(uv_stream_t* server, int status) {
    if (status < 0) return;
    iems_app_t* app = (iems_app_t*)server->data;
    uv_pipe_t* client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t*)client) == 0 && app && app->hub) {
        int cfd;
        uv_fileno((const uv_handle_t*)client, &cfd);
        edge_fd_send(cfd, datahub_get_ring_fd(app->hub));
        edge_fd_send(cfd, datahub_get_shadow_fd(app->hub));
    }
    uv_close((uv_handle_t*)client, (uv_close_cb)free);
}

static void _on_north_connect(uv_stream_t* server, int status) {
    if (status < 0) return;
    north_accept_ctx_t* ctx = (north_accept_ctx_t*)server->data;
    iems_app_t* app = (iems_app_t*)server->loop->data;
    uv_pipe_t* client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t*)client) == 0 && app && app->orch) {
        int fd;
        uv_fileno((const uv_handle_t*)client, &fd);
        int new_fd = dup(fd);
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t* h =
                io_executor_create_from_fd(server->loop, new_fd, datahub_get_io_pool(app->hub));
            if (h) {
                bus_orchestrator_attach_io(app->orch, ctx->bus_id, h);
                if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
            } else {
                close(new_fd);
            }
        }
    }
    uv_close((uv_handle_t*)client, (uv_close_cb)free);
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

static pipeline_loader_ctx_t _iems_loader_ctx(iems_app_t* app) {
    pipeline_loader_ctx_t ctx = {
        .db = stream_sqlite_port_from_sqlite(app->db_service),
        .control_ops = {.ctx = app, .submit_control = _iems_stream_submit_control},
        .audit_secret = getenv("VPP_AUDIT_SECRET"),
        .production = _iems_production_env(),
        .params = NULL
    };
    return ctx;
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
    app->drv_mgr = driver_manager_create();
    iems_driver_registry_init(app->drv_mgr);

    app->orch = bus_orchestrator_create(app->hub, app->arb, app->drv_mgr, 256, 256, 2048);
    datahub_bind_io_pool(app->hub, bus_orchestrator_get_pool(app->orch));
    edge_api_init(app->orch, app->drv_mgr);

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
    northbound_mgr_init_ex(app->north, nb_source, "IEMS_V4", app->db_service, transport);
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
    app->svc = service_new(loop, app->hub, app->arb, app->db_service, app->stream_eng, NULL,
                           app->north);
    int rest_port = (cfg && cfg->rest_port > 0) ? cfg->rest_port : 9091;
    rest_server_create(loop, app->svc, rest_port);

    const char* ui_path =
        (cfg && cfg->ui_sock && cfg->ui_sock[0]) ? cfg->ui_sock : EDGE_UI_SOCKET_PATH;
    app->ui_srv = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, app->ui_srv, 0);
    app->ui_srv->data = app;
    unlink(ui_path);
    int ui_bind = uv_pipe_bind(app->ui_srv, ui_path);
    int ui_listen =
        (ui_bind == 0) ? uv_listen((uv_stream_t*)app->ui_srv, 128, on_ui_connect) : 0;
    if (ui_bind == 0 && ui_listen == 0) {
        printf("\033[32m[IEMS-INIT] UI Companion Socket listening at %s\033[0m\n", ui_path);
    } else {
        // 路径太长/目录不存在/被占:UI 面板会看不到数据,得让现场直接看见原因
        LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: UI Companion Socket FAILED at %s (bind=%s)", ui_path,
                  uv_strerror(ui_bind));
        if (ui_bind == 0) LOG_ERROR(LOG_CAT_CORE, "IEMS-INIT: UI listen FAILED: %s",
                                    uv_strerror(ui_listen));
        if (ui_bind == 0) unlink(ui_path);  // bind 成功、listen 失败会留下没人听的 socket 文件
        uv_close((uv_handle_t*)app->ui_srv, (uv_close_cb)free);
        app->ui_srv = NULL;
    }

    int north_bound_seen = 0;
    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* c = &conns[i];
        if (strcmp(c->category, "NORTHBOUND") == 0 && c->id < IEMS_MAX_VBUS) {
            // cfg->north_sock 只覆盖第一条 NORTHBOUND(单总线是常态);其余条目用契约表
            // 自带的 simulation_path,否则第二条会 unlink 掉第一条正在监听的 socket
            bool from_env = (north_bound_seen == 0 && cfg && cfg->north_sock && cfg->north_sock[0]);
            const char* path = NULL;
            if (from_env) {
                path = cfg->north_sock;
            } else if (c->simulation_path && c->simulation_path[0]) {
                path = c->simulation_path;
                if (north_bound_seen > 0) {
                    LOG_WARN(LOG_CAT_CORE,
                             "IEMS-INIT: NORTHBOUND bus %d uses contract path %s "
                             "(IEMS_NORTH_SOCK 只覆盖第一条北向总线)",
                             (int)c->id, path);
                }
            } else if (north_bound_seen == 0) {
                path = "/tmp/iems_north_slave.sock";
            } else {
                // 共用同一个 socket 会让后面的 unlink 掉前面正在监听的文件,宁可不监听
                LOG_ERROR(LOG_CAT_CORE,
                          "IEMS-INIT: NORTHBOUND bus %d has no path in contract table; skipped "
                          "(refusing to share another bus's socket)",
                          (int)c->id);
                continue;
            }
            north_bound_seen++;

            app->north_srv[c->id] = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
            uv_pipe_init(loop, app->north_srv[c->id], 0);
            unlink(path);

            int nb_bind = uv_pipe_bind(app->north_srv[c->id], path);
            int nb_listen = (nb_bind == 0)
                                ? uv_listen((uv_stream_t*)app->north_srv[c->id], 128,
                                            _on_north_connect)
                                : nb_bind;
            if (nb_bind == 0 && nb_listen == 0) {
                app->north_ctx[c->id].bus_id = c->id;
                app->north_ctx[c->id].bus_idx = c->id % IEMS_MAX_VBUS;
                app->north_srv[c->id]->data = &app->north_ctx[c->id];
                app->bus_state[app->north_ctx[c->id].bus_idx] = BUS_STATE_LISTENING;
                printf("\033[32m[IEMS-INIT] Northbound Slave LISTENING at %s\033[0m\n", path);
            } else {
                LOG_ERROR(LOG_CAT_CORE,
                          "IEMS-INIT: Northbound Slave FAILED at %s (bind=%s, listen=%s)", path,
                          uv_strerror(nb_bind), uv_strerror(nb_listen));
                if (nb_bind == 0) unlink(path);
                uv_close((uv_handle_t*)app->north_srv[c->id], (uv_close_cb)free);
                app->north_srv[c->id] = NULL;
            }
        }
    }

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
    iems_internal_bus_stop(&app->internal_bus);

    if (app->ui_srv) {
        uv_close((uv_handle_t*)app->ui_srv, (uv_close_cb)free);
    }
    for (int i = 0; i < IEMS_MAX_VBUS; i++) {
        if (app->north_srv[i]) {
            uv_close((uv_handle_t*)app->north_srv[i], (uv_close_cb)free);
        }
    }

    uv_run(app->loop, UV_RUN_NOWAIT);

    iems_internal_bus_join(&app->internal_bus);

    if (app->stream_eng) stream_engine_destroy(app->stream_eng);
    wasm_mgr_cleanup();
    if (app->svc) service_delete(app->svc);
    if (app->db_service) sqlite_svc_destroy(app->db_service);
    if (app->orch) bus_orchestrator_destroy(app->orch);
    if (app->arb) arbiter_destroy(app->arb);
    if (app->drv_mgr) driver_manager_destroy(app->drv_mgr);
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
