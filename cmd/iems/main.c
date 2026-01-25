#include <curl/curl.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "contracts.h"
#include "edge_api.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "queue.h"
#include "rest_server.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"

#define MAX_VBUS 32

typedef enum {
    BUS_STATE_DISCONNECTED = 0,
    BUS_STATE_CONNECTING,
    BUS_STATE_CONNECTED
} bus_conn_state_t;

typedef struct {
    service_t *svc;
    datahub_t *hub;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    stream_engine_t *stream_eng;
    northbound_mgr_t *north;
    sqlite_svc_t *db_service;
    char *db_path;
} iems_app_t;

static iems_app_t g_app = {0};
static struct uv__queue g_poll_task_head;
static bool g_assets_loaded = false;
static bus_conn_state_t g_bus_state[MAX_VBUS] = {BUS_STATE_DISCONNECTED};

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    char protocol[16];
    uint32_t interval_ms;
    uint64_t last_run_ms;
    struct uv__queue node;
} iems_poll_task_t;

static void on_poll_complete(int status, const pipe_msg_t *result) {
    if (status == 0 && result) {
        printf("\033[92m[IEMS-DATA] Polled: Tag=%u Val=%.2f -> Publishing to DataHub\033[0m\n",
               result->tag_id, result->payload.val.f);
        datahub_publish(g_app.hub, result);
    }
}

static void on_ui_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int cfd;
        uv_fileno((const uv_handle_t *)client, &cfd);
        edge_fd_send(cfd, shmring_get_fd(&g_app.hub->ring));
        edge_fd_send(cfd, g_app.hub->shadow_fd);
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

static void iems_scheduler_tick(uv_periodic_t *handle) {
    if (!g_assets_loaded) return;
    uint64_t now = uv_now(handle->timer.loop);
    struct uv__queue *q;
    uv__queue_foreach(q, &g_poll_task_head) {
        iems_poll_task_t *task = uv__queue_data(q, iems_poll_task_t, node);
        if (now - task->last_run_ms >= (uint64_t)task->interval_ms) {
            if (g_bus_state[task->bus_id % MAX_VBUS] == BUS_STATE_CONNECTED) {
                edge_api_read_tag(task->bus_id, task->protocol, task->slave_id, task->tag_id,
                                  on_poll_complete);
            }
            task->last_run_ms = now;
        }
    }
}

static int _load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)cols;
    if (argc < 4) return 0;
    int bus_id = argv[0] ? atoi(argv[0]) : 0;
    uint32_t slave_id = argv[1] ? (uint32_t)atoi(argv[1]) : 1;
    uint32_t tag_id = argv[2] ? (uint32_t)atoi(argv[2]) : 0;
    const char *proto = argv[3] ? argv[3] : "ModbusRTU";

    // 查重：防止重复加载
    struct uv__queue *q;
    uv__queue_foreach(q, &g_poll_task_head) {
        iems_poll_task_t *existing = uv__queue_data(q, iems_poll_task_t, node);
        if (existing->bus_id == bus_id && existing->tag_id == tag_id) return 0;
    }

    iems_poll_task_t *task = calloc(1, sizeof(iems_poll_task_t));
    task->bus_id = bus_id;
    task->slave_id = slave_id;
    task->tag_id = tag_id;
    strncpy(task->protocol, proto, 15);

    // 从契约元数据获取轮询间隔
    task->interval_ms = 1000;
    for (uint32_t i = 0; i < g_tag_metadata_len; i++) {
        if (g_tag_metadata[i].id == tag_id) {
            if (g_tag_metadata[i].poll_interval_ms > 0) {
                task->interval_ms = g_tag_metadata[i].poll_interval_ms;
            }
            break;
        }
    }

    uv__queue_insert_tail(&g_poll_task_head, &task->node);
    printf("\033[93m[IEMS-LOAD] Task: Bus=%d, Slave=%u, Tag=%u, Proto=%s, Interval=%u ms\033[0m\n",
           task->bus_id, task->slave_id, task->tag_id, task->protocol, task->interval_ms);
    return 0;
}

static void iems_load_tasks_timer_cb(uv_timer_t *handle) {
    sqlite3 *raw_db = NULL;
    if (sqlite3_open_v2(g_app.db_path, &raw_db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        if (raw_db) sqlite3_close(raw_db);
        return;
    }

    const char *check_sql = "SELECT val FROM matrix_meta WHERE key='is_ready';";
    sqlite3_stmt *stmt;
    bool ready = false;
    if (sqlite3_prepare_v2(raw_db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            if (strcmp((const char *)sqlite3_column_text(stmt, 0), "1") == 0) ready = true;
        }
        sqlite3_finalize(stmt);
    }

    if (ready) {
        const char *sql =
            "SELECT t.bus_id, i.addr, t.tag_id, i.protocol FROM poll_tasks t "
            "INNER JOIN inventory i ON t.device_id = i.id;";
        char *err = NULL;
        if (sqlite3_exec(raw_db, sql, _load_tasks_cb, NULL, &err) == SQLITE_OK) {
            int count = 0;
            struct uv__queue *q;
            uv__queue_foreach(q, &g_poll_task_head) count++;
            if (count > 0) {
                printf("\033[32m[IEMS-INIT] %d tasks loaded from DB. Polling active.\033[0m\n",
                       count);
                g_assets_loaded = true;
                uv_timer_stop(handle);
                uv_close((uv_handle_t *)handle, (uv_close_cb)free);
            }
        } else {
            printf("[IEMS-ERROR] SQL Load Error: %s\n", err);
            if (err) sqlite3_free(err);
        }
    }
    sqlite3_close(raw_db);
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    int bus_id = (int)(intptr_t)req->data;
    if (status < 0) {
        printf("\033[31m[IEMS-INIT] VBUS %d Connect Failed: %s\033[0m\n", bus_id,
               uv_strerror(status));
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        g_bus_state[bus_id % MAX_VBUS] = BUS_STATE_DISCONNECTED;
        free(req);
        return;
    }
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    printf("\033[32m[IEMS-INIT] VBUS %d Handshake Success (FD:%d)\033[0m\n", bus_id, fd);

    io_handle_t *h = io_executor_create_from_fd(req->handle->loop, fd, g_app.hub->io_pool);
    if (h) {
        bus_orchestrator_attach_io(g_app.orch, bus_id, h);
        if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
        g_bus_state[bus_id % MAX_VBUS] = BUS_STATE_CONNECTED;
    } else {
        g_bus_state[bus_id % MAX_VBUS] = BUS_STATE_DISCONNECTED;
    }
    free(req);
}

static void _try_connect_buses(uv_timer_t *handle) {
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        if (strcmp(c->category, "SOUTHBOUND") != 0 && strcmp(c->category, "NORTHBOUND") != 0)
            continue;

        int bus_idx = c->id % MAX_VBUS;
        if (g_bus_state[bus_idx] != BUS_STATE_DISCONNECTED) continue;

        const char *path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            printf("[IEMS-INIT] Attempting to connect VBUS %d -> %s\n", c->id, path);
            uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
            uv_pipe_init(handle->loop, pipe, 0);
            uv_connect_t *req = malloc(sizeof(uv_connect_t));
            req->data = (void *)(intptr_t)c->id;
            g_bus_state[c->id % MAX_VBUS] = BUS_STATE_CONNECTING;
            uv_pipe_connect(req, pipe, path, on_vbus_connect);
        }
    }
}

extern void adc_driver_handle_connection(int sock_fd, datahub_t *hub);

typedef struct {
    int fd;
    datahub_t *hub;
} adc_thread_ctx_t;

static void _adc_thread_entry(void *arg) {
    adc_thread_ctx_t *ctx = (adc_thread_ctx_t *)arg;
    adc_driver_handle_connection(ctx->fd, ctx->hub);
    free(ctx);
}

static void on_adc_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        free(req);
        return;
    }
    printf(
        "\033[32m[IEMS-INIT] Internal High-Speed Channel Connected. Launching ADC "
        "Thread...\033[0m\n");

    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);

    // [Sprint 5] 异步化：在独立线程处理高频流，避免主循环拥塞
    adc_thread_ctx_t *ctx = malloc(sizeof(adc_thread_ctx_t));
    ctx->fd = dup(fd);  // DUP 以脱离 libuv 生命周期管理
    ctx->hub = g_app.hub;

    uv_thread_t tid;
    uv_thread_create(&tid, _adc_thread_entry, ctx);

    // 关闭 libuv 层面的句柄，不影响已 DUP 的 FD
    uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
    free(req);
}

static void _try_connect_internal_buses(uv_timer_t *handle) {
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        if (strcmp(c->category, "INTERNAL") != 0) continue;

        const char *path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
            uv_pipe_init(handle->loop, pipe, 1);  // 启用 FD 传递支持
            uv_connect_t *req = malloc(sizeof(uv_connect_t));
            uv_pipe_connect(req, pipe, path, on_adc_connect);
        }
    }
}

static void iems_shutdown(uv_loop_t *loop) {
    printf("\n🛑 [IEMS] Shutdown initiated...\n");
    if (g_app.svc) service_delete(g_app.svc);
    if (g_app.stream_eng) stream_engine_destroy(g_app.stream_eng);
    if (g_app.north) {
        northbound_mgr_stop(g_app.north);
        free(g_app.north);
    }
    if (g_app.db_service) sqlite_svc_destroy(g_app.db_service);
    if (g_app.orch) bus_orchestrator_destroy(g_app.orch);
    if (g_app.drv_mgr) driver_manager_destroy(g_app.drv_mgr);
    if (g_app.hub) {
        datahub_destroy(g_app.hub);
        free(g_app.hub);
    }
    if (g_app.db_path) free(g_app.db_path);
    curl_global_cleanup();

    struct uv__queue *q;
    while (!uv__queue_empty(&g_poll_task_head)) {
        q = uv__queue_head(&g_poll_task_head);
        iems_poll_task_t *task = uv__queue_data(q, iems_poll_task_t, node);
        uv__queue_remove(q);
        free(task);
    }
    printf("✨ [IEMS] Resources cleaned. Goodbye.\n");
}

static void on_signal_exit(uv_signal_t *handle, int signum) {
    printf("\n🛑 [IEMS] Termination Signal %d Received.\n", signum);
    uv_stop(handle->loop);
}

int main(int argc, char *argv[]) {
    // 0. 全局初始化 (必须在主线程且启动任何子线程前)
    curl_global_init(CURL_GLOBAL_ALL);

    uv_loop_t *loop = uv_default_loop();
    uv__queue_init(&g_poll_task_head);

    static uv_signal_t s1, s2;
    uv_signal_init(loop, &s1);
    uv_signal_start(&s1, on_signal_exit, SIGINT);
    uv_signal_init(loop, &s2);
    uv_signal_start(&s2, on_signal_exit, SIGTERM);

    printf("\n--- EdgeOS IEMS Omnis Edition ---\n");

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    g_app.drv_mgr = driver_manager_create();
    modbus_driver_register(g_app.drv_mgr);

    const char *shm_name = getenv("EDGE_SHM_NAME") ? getenv("EDGE_SHM_NAME") : EDGE_SHM_BUS_NAME;
    g_app.hub = calloc(1, sizeof(datahub_t));
    datahub_init(g_app.hub, loop, shm_name, 4096, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);

    g_app.orch = bus_orchestrator_create(g_app.hub, arbiter, g_app.drv_mgr, 8, 32, 2048);
    g_app.hub->io_pool = bus_orchestrator_get_pool(g_app.orch);
    edge_api_init(g_app.orch, g_app.drv_mgr);

    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        const driver_contract_injection_t *inj = edge_model_find("ModbusRTU");
        if (!inj) continue;

        if (strcmp(c->category, "SOUTHBOUND") == 0) {
            bus_orchestrator_setup_master(g_app.orch, c->id, "ModbusRTU", (uint32_t)c->id, inj);
            printf("[IEMS-INIT] Bus %d initialized as MASTER (Contract 'ModbusRTU')\n", c->id);
        } else if (strcmp(c->category, "NORTHBOUND") == 0) {
            bus_orchestrator_setup_slave(g_app.orch, c->id, "ModbusRTU", (uint32_t)c->id, inj);
            printf("[IEMS-INIT] Bus %d initialized as SLAVE (Contract 'ModbusRTU')\n", c->id);
        }
    }

    const char *db_env = getenv("EDGE_DB_PATH");
    g_app.db_path = strdup(db_env ? db_env : "iems.db");
    printf("[IEMS-INIT] Connecting to Matrix DB: %s\n", g_app.db_path);

    sqlite_config_t db_conf = {.db_path = g_app.db_path, .strategy = STRATEGY_LITE};
    g_app.db_service = sqlite_svc_create(loop, &db_conf);

    // 4. [Calculus] 计算平面：挂载 Stream Engine 并注入 DataHub 通知

    g_app.stream_eng = stream_engine_create(g_app.hub, loop, g_app.orch);

    pipeline_loader_ctx_t loader_ctx = {

        .db = g_app.db_service,

        .orch = g_app.orch,

        .params = NULL

    };

    stream_engine_load_static_metadata(g_app.stream_eng, g_pipeline_metadata,
                                       g_pipeline_metadata_len, &loader_ctx);

    stream_engine_start(g_app.stream_eng);

    // 5. [Orchestration] 编排与主循环配置

    g_app.north = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(g_app.north, loop, &g_app.hub->ring, "IEMS_V4", g_app.db_service);
    printf("[IEMS-INIT] Northbound Manager configured at %p\n", (void *)g_app.north);

    g_app.svc = service_new(loop, g_app.hub, arbiter, g_app.db_service, g_app.stream_eng, NULL,
                            g_app.north);
    int rest_port = getenv("EDGE_REST_PORT") ? atoi(getenv("EDGE_REST_PORT")) : 9091;
    rest_server_create(loop, g_app.svc, rest_port);
    rest_server_enable_write(true);

    unlink(EDGE_UI_SOCKET_PATH);
    uv_pipe_t *ui_srv = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ui_srv, 0);
    if (uv_pipe_bind(ui_srv, EDGE_UI_SOCKET_PATH) == 0) {
        uv_listen((uv_stream_t *)ui_srv, 10, on_ui_connect);
        printf("[IEMS-INIT] UI Discovery Socket ACTIVE at %s\n", EDGE_UI_SOCKET_PATH);
    }

    uv_timer_t *st = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, st);
    uv_timer_start(st, iems_load_tasks_timer_cb, 50, 200);

    static uv_timer_t vt;
    uv_timer_init(loop, &vt);
    uv_timer_start(&vt, _try_connect_buses, 100, 1000);

    static uv_timer_t it;
    uv_timer_init(loop, &it);
    uv_timer_start(&it, _try_connect_internal_buses, 100, 1000);

    static uv_periodic_t sched_h;
    uv_periodic_init(loop, &sched_h);
    uv_periodic_set(&sched_h, 0, 100.0, NULL);
    uv_periodic_start(&sched_h, iems_scheduler_tick);

    arbiter_resume(arbiter);
    int ret = service_run(g_app.svc);
    iems_shutdown(loop);
    return ret;
}
