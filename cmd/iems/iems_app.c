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
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_log.h"
#include "edge_service.h"
#include "iems_driver_adapter.h"
#include "iems_task_loader.h"
#include "io_executor.h"
#include "modbus_codec.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "queue.h"
#include "rest_server.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "wasm_mgr.h"

#define MAX_VBUS 32

typedef enum {
    BUS_STATE_DISCONNECTED = 0,
    BUS_STATE_CONNECTING,
    BUS_STATE_CONNECTED,
    BUS_STATE_LISTENING
} bus_conn_state_t;

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    int dense_idx;
    char protocol[16];
    uint32_t interval_ms;
    uv_periodic_t scheduler;
    iems_node_t* agg_root;
    struct uv__queue node;
} iems_poll_task_t;

typedef struct {
    int bus_id;
    int bus_idx;
} north_accept_ctx_t;

typedef struct {
    int fd;
    datahub_t* hub;
} adc_thread_ctx_t;

struct iems_app_s {
    uv_loop_t* loop;
    iems_node_t* node;
    service_t* svc;
    datahub_t* hub;
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    driver_manager_t* drv_mgr;
    stream_engine_t* stream_eng;
    northbound_mgr_t* north;
    sqlite_svc_t* db_service;
    char* db_path;

    struct uv__queue poll_task_head;
    bool assets_loaded;
    bus_conn_state_t bus_state[MAX_VBUS];

    uv_timer_t* timer_load_tasks;
    uv_timer_t timer_vbus;
    uv_timer_t timer_internal;
    uv_timer_t timer_monitor;

    volatile bool adc_running;
    uv_thread_t adc_tid;

    uv_pipe_t* ui_srv;
    uv_pipe_t* north_srv[MAX_VBUS];
    north_accept_ctx_t north_ctx[MAX_VBUS];
};

volatile bool g_adc_running = false;

static void on_poll_complete(int status, const pipe_msg_t* result, void* user_data) {
    // user_data 是聚合根(比 arbiter 活得久),不是 poll task —— 关停时 task 会先被释放。
    iems_node_t* node = (iems_node_t*)user_data;
    if (status != 0 || !result || !node) return;
    if (result->type != PIPEMSG_TYPE_SCALAR) return;
    // 镜像只能由聚合根写;这里只转交采样,不做任何堆分配。
    if (iems_node_ingest_sample(node, result->tag_id, result->payload.val.f) != 0) {
        static uint64_t last_warn_ns;  // 热路径:失败日志全局限流到 60s 一次
        uint64_t now = uv_hrtime();
        if (now - last_warn_ns > 60ULL * 1000000000ULL) {
            last_warn_ns = now;
            LOG_WARN(LOG_CAT_CORE, "IEMS ingest failed for tag %u", result->tag_id);
        }
    }
}

/**
 * @brief [Domain Boundary Guard] 控制写路径强制经聚合根 (Issue #400)
 */
static int _iems_stream_submit_control(void* ctx, int bus_id, uint32_t tag_id, double value,
                                       const char* proto) {
    iems_app_t* app = (iems_app_t*)ctx;
    if (!app || !app->node) return -1;

    // 💡 [ARCH-GATE] 商业储能 PCS 功率设定或应急控制必须经由聚合根领域方法校验
    if (tag_id == IEMS_TAG_PCS_CONTROL) {
        double actual_kw = 0.0;
        int rc = iems_node_dispatch_power(app->node, value, &actual_kw);
        if (rc < 0) {
            LOG_WARN(LOG_CAT_CORE, "IEMS control write rejected by aggregate root (rc=%d)", rc);
            return rc;
        }
        value = actual_kw;
    } else {
        if (app->hub) {
            pipe_msg_t ctrl_msg = {0};
            pipe_msg_init_scalar(&ctrl_msg, tag_id, value, uv_hrtime());
            datahub_publish(app->hub, &ctrl_msg);
        }
    }

    // 经领域授权与约束校验后，下发物理总线
    return edge_api_write_tag(bus_id, proto, 1, tag_id, value, NULL, NULL);
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

static void on_poll_task_trigger(uv_periodic_t* handle) {
    iems_poll_task_t* task = (iems_poll_task_t*)handle->data;
    if (!task) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t* conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    bool is_southbound = false;
    for (uint32_t i = 0; i < conn_count; i++) {
        if (conns[i].id == task->bus_id &&
            strcmp(conns[i].category, "SOUTHBOUND") == 0) {
            is_southbound = true;
            break;
        }
    }

    if (is_southbound) {
        edge_api_read_tag(task->bus_id, task->protocol, task->slave_id, task->tag_id,
                          on_poll_complete, task->agg_root);
    }
}

static void _on_iems_task_loaded(const iems_loaded_task_t* loaded, void* user_data) {
    iems_app_t* app = (iems_app_t*)user_data;
    if (!loaded || !app) return;

    struct uv__queue* q;
    uv__queue_foreach(q, &app->poll_task_head) {
        iems_poll_task_t* existing = uv__queue_data(q, iems_poll_task_t, node);
        if (existing->bus_id == loaded->bus_id && existing->slave_id == loaded->slave_id &&
            existing->tag_id == loaded->tag_id)
            return;
    }

    iems_poll_task_t* task = calloc(1, sizeof(iems_poll_task_t));
    task->bus_id = loaded->bus_id;
    task->slave_id = loaded->slave_id;
    task->tag_id = loaded->tag_id;
    task->dense_idx = edge_tag_get_compact_index(loaded->tag_id);
    task->agg_root = app->node;
    strncpy(task->protocol, loaded->protocol, 15);
    task->interval_ms = 1000;

    const edge_tag_meta_t* meta = iems_get_tag_meta(loaded->tag_id);
    if (meta && meta->poll_interval_ms > 0) {
        task->interval_ms = meta->poll_interval_ms;
    }

    printf("📝 [POLL REGISTER] Tag: %u, Bus: %d, Protocol: %s, Interval: %d\n", task->tag_id,
           task->bus_id, task->protocol, task->interval_ms);

    uv__queue_insert_tail(&app->poll_task_head, &task->node);
    uv_periodic_init(app->loop, &task->scheduler);
    task->scheduler.data = task;
    uv_periodic_set(&task->scheduler, 0, (double)task->interval_ms, NULL);
    uv_periodic_start(&task->scheduler, on_poll_task_trigger);
}

static void iems_load_tasks_timer_cb(uv_timer_t* handle) {
    iems_app_t* app = (iems_app_t*)handle->data;
    if (!app || !app->db_service) return;

    int loaded = iems_poll_tasks_load(app->db_service, _on_iems_task_loaded, app);
    if (loaded > 0) {
        app->assets_loaded = true;
        uv_timer_stop(handle);
        uv_close((uv_handle_t*)handle, (uv_close_cb)free);
        app->timer_load_tasks = NULL;
    }
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

static void _try_connect_buses(uv_timer_t* handle) {
    iems_app_t* app = (iems_app_t*)handle->data;
    if (!app) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t* conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* c = &conns[i];
        if (c->id >= MAX_VBUS) continue;

        int bus_idx = (int)c->id;
        if (app->bus_state[bus_idx] != BUS_STATE_DISCONNECTED) continue;

        char env_key[32];
        snprintf(env_key, sizeof(env_key), "OMNIS_BUS_%d_DEV", bus_idx);
        const char* env_path = getenv(env_key);
        const char* path = env_path ? env_path : edge_resolve_bus_path(c->id);
        if (!path) continue;

        const char* search_paths[3];
        search_paths[0] = env_path;
        search_paths[1] = path;
        char fallback_path[64];
        snprintf(fallback_path, sizeof(fallback_path), "/tmp/vbus%d.sock", bus_idx);
        search_paths[2] = fallback_path;

        for (int p_idx = 0; p_idx < 3; p_idx++) {
            const char* cur_path = search_paths[p_idx];
            if (!cur_path || cur_path[0] == '\0') continue;

            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) continue;

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, cur_path, sizeof(addr.sun_path) - 1);

            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                io_handle_t* h =
                    io_executor_create_from_fd(handle->loop, fd, datahub_get_io_pool(app->hub));
                if (h) {
                    bus_orchestrator_attach_io(app->orch, c->id, h);
                    if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
                    app->bus_state[bus_idx] = BUS_STATE_CONNECTED;
                    printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via PATH: %s\033[0m\n",
                           bus_idx, cur_path);
                    break;
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }
    }
}

extern void adc_driver_handle_connection(int sock_fd, datahub_t* hub);

static void _adc_thread_entry(void* arg) {
    adc_thread_ctx_t* ctx = (adc_thread_ctx_t*)arg;
    adc_driver_handle_connection(ctx->fd, ctx->hub);
    g_adc_running = false;
    free(ctx);
}

static void _try_connect_internal_buses(uv_timer_t* handle) {
    iems_app_t* app = (iems_app_t*)handle->data;
    if (!app || g_adc_running) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t* conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* c = &conns[i];
        if (strcmp(c->category, "INTERNAL") != 0 || g_adc_running) continue;
        const char* path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) continue;
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                adc_thread_ctx_t* ctx = malloc(sizeof(adc_thread_ctx_t));
                ctx->fd = fd;
                ctx->hub = app->hub;
                g_adc_running = true;
                uv_thread_create(&app->adc_tid, _adc_thread_entry, ctx);
            } else {
                close(fd);
            }
        }
    }
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

static void _on_task_close(uv_handle_t* handle) {
    uv_periodic_t* periodic = (uv_periodic_t*)handle->data;
    iems_poll_task_t* task = (iems_poll_task_t*)periodic->data;
    free(task);
}

iems_app_t* iems_app_assemble(uv_loop_t* loop, const iems_app_config_t* cfg) {
    if (!loop) return NULL;

    iems_app_t* app = (iems_app_t*)calloc(1, sizeof(iems_app_t));
    if (!app) return NULL;

    app->loop = loop;
    loop->data = app;
    uv__queue_init(&app->poll_task_head);

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
    app->node = iems_node_create(loop, &node_cfg);
    if (!app->node) {
        LOG_ERROR(LOG_CAT_CORE, "IEMS Assembler: Failed to create iems_node aggregate root");
        free(app);
        return NULL;
    }
    app->hub = iems_node_get_datahub(app->node);

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
    const driver_contract_injection_t* inj = iems_get_driver_injection("ModbusRTU");

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* conn = &conns[i];
        if (strcmp(conn->category, "SOUTHBOUND") == 0) {
            if (inj) {
                bus_orchestrator_setup_master(app->orch, conn->id, "ModbusRTU",
                                              (uint32_t)conn->id, inj);
            }
        } else if (strcmp(conn->category, "NORTHBOUND") == 0) {
            if (inj) {
                bus_orchestrator_setup_slave(app->orch, conn->id, "ModbusRTU", 1, inj);
            }
        }
    }

    // =========================================================================
    // Phase 4: 本地持久化与流计算引擎 (SQLite & StreamEngine)
    // =========================================================================
    const char* db_file = (cfg && cfg->db_path) ? cfg->db_path : "iems.db";
    app->db_path = strdup(db_file);
    sqlite_config_t db_conf = {.db_path = app->db_path, .strategy = STRATEGY_LITE};
    app->db_service = sqlite_svc_create(loop, &db_conf);

    app->stream_eng =
        stream_engine_create(app->hub, NULL, app->orch, edge_stage_factory_get_registry());
    pipeline_loader_ctx_t loader_ctx = {
        .db = app->db_service,
        .control_ops = {.ctx = app, .submit_control = _iems_stream_submit_control},
        .params = NULL
    };
    uint32_t pipeline_count = 0;
    const edge_pipeline_def_t* pipelines = iems_get_pipelines(&pipeline_count);
    stream_engine_load_static_metadata(app->stream_eng, pipelines, (int)pipeline_count, &loader_ctx);
    stream_engine_start(app->stream_eng);

    // =========================================================================
    // Phase 5: 微运行时与北向通信 (WASM & Northbound MQTT)
    // =========================================================================
    wasm_mgr_init(app->hub);

    app->north = (northbound_mgr_t*)calloc(1, sizeof(northbound_mgr_t));
    const char* mqtt_url = (cfg && cfg->mqtt_url) ? cfg->mqtt_url : "tcp://localhost:1883";
    nb_transport_ops_t transport = mqtt_producer_create(mqtt_url, "IEMS_EDGE_NODE");
    nb_source_ops_t nb_source = {.ctx = app->hub, .read_next = (void*)(datahub_read_ring)};
    northbound_mgr_init_ex(app->north, nb_source, "IEMS_V4", app->db_service, transport);

    // =========================================================================
    // Phase 6: 系统服务、REST API 与 UI 伴随通道
    // =========================================================================
    app->svc = service_new(loop, app->hub, app->arb, app->db_service, app->stream_eng, NULL,
                           app->north);
    int rest_port = (cfg && cfg->rest_port > 0) ? cfg->rest_port : 9091;
    rest_server_create(loop, app->svc, rest_port);

    const char* ui_path = EDGE_UI_SOCKET_PATH;
    app->ui_srv = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, app->ui_srv, 0);
    app->ui_srv->data = app;
    unlink(ui_path);
    if (uv_pipe_bind(app->ui_srv, ui_path) == 0) {
        uv_listen((uv_stream_t*)app->ui_srv, 128, on_ui_connect);
        printf("\033[32m[IEMS-INIT] UI Companion Socket listening at %s\033[0m\n", ui_path);
    }

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t* c = &conns[i];
        if (strcmp(c->category, "NORTHBOUND") == 0 && c->id < MAX_VBUS) {
            char env_key[32];
            snprintf(env_key, sizeof(env_key), "OMNIS_BUS_%d_DEV", c->id);
            const char* path = getenv(env_key);
            if (!path) path = "/tmp/iems_north_slave.sock";

            app->north_srv[c->id] = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
            uv_pipe_init(loop, app->north_srv[c->id], 0);
            unlink(path);

            if (uv_pipe_bind(app->north_srv[c->id], path) == 0 &&
                uv_listen((uv_stream_t*)app->north_srv[c->id], 128, _on_north_connect) == 0) {
                app->north_ctx[c->id].bus_id = c->id;
                app->north_ctx[c->id].bus_idx = c->id % MAX_VBUS;
                app->north_srv[c->id]->data = &app->north_ctx[c->id];
                app->bus_state[app->north_ctx[c->id].bus_idx] = BUS_STATE_LISTENING;
                printf("\033[32m[IEMS-INIT] Northbound Slave LISTENING at %s\033[0m\n", path);
            } else {
                uv_close((uv_handle_t*)app->north_srv[c->id], (uv_close_cb)free);
                app->north_srv[c->id] = NULL;
            }
        }
    }

    return app;
}

int iems_app_start(iems_app_t* app) {
    if (!app) return -1;

    iems_node_start(app->node);

    app->timer_load_tasks = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(app->loop, app->timer_load_tasks);
    app->timer_load_tasks->data = app;
    uv_timer_start(app->timer_load_tasks, iems_load_tasks_timer_cb, 50, 200);

    uv_timer_init(app->loop, &app->timer_monitor);
    app->timer_monitor.data = app;
    uv_timer_start(&app->timer_monitor, _monitor_shadow_tick, 1000, 1000);

    uv_timer_init(app->loop, &app->timer_vbus);
    app->timer_vbus.data = app;
    uv_timer_start(&app->timer_vbus, _try_connect_buses, 0, 500);

    uv_timer_init(app->loop, &app->timer_internal);
    app->timer_internal.data = app;
    uv_timer_start(&app->timer_internal, _try_connect_internal_buses, 0, 500);

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

    if (app->timer_load_tasks) {
        uv_timer_stop(app->timer_load_tasks);
        uv_close((uv_handle_t*)app->timer_load_tasks, (uv_close_cb)free);
    }
    uv_timer_stop(&app->timer_monitor);
    uv_close((uv_handle_t*)&app->timer_monitor, NULL);

    uv_timer_stop(&app->timer_vbus);
    uv_close((uv_handle_t*)&app->timer_vbus, NULL);

    uv_timer_stop(&app->timer_internal);
    uv_close((uv_handle_t*)&app->timer_internal, NULL);

    while (!uv__queue_empty(&app->poll_task_head)) {
        struct uv__queue* q = uv__queue_head(&app->poll_task_head);
        iems_poll_task_t* task = uv__queue_data(q, iems_poll_task_t, node);
        uv__queue_remove(q);
        uv_periodic_stop(&task->scheduler);
        uv_close((uv_handle_t*)&task->scheduler.timer, _on_task_close);
    }

    if (app->ui_srv) {
        uv_close((uv_handle_t*)app->ui_srv, (uv_close_cb)free);
    }
    for (int i = 0; i < MAX_VBUS; i++) {
        if (app->north_srv[i]) {
            uv_close((uv_handle_t*)app->north_srv[i], (uv_close_cb)free);
        }
    }

    uv_run(app->loop, UV_RUN_NOWAIT);

    if (g_adc_running) {
        g_adc_running = false;
        uv_thread_join(&app->adc_tid);
    }

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
