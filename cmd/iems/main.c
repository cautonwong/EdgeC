#include <curl/curl.h>
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

#include "contracts.h"
#include "edge_api.h"
#include "edge_service.h"
#include "modbus_codec.h"
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
    BUS_STATE_CONNECTED,
    BUS_STATE_LISTENING
} bus_conn_state_t;

typedef struct {
    service_t *svc;
    datahub_t *hub;
    arbiter_t *arb;
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

static uv_timer_t *g_timer_load_tasks = NULL;
static uv_timer_t *g_timer_vbus = NULL;
static uv_timer_t *g_timer_internal = NULL;

volatile bool g_adc_running = false;
static uv_thread_t g_adc_tid;

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    char protocol[16];
    uint32_t interval_ms;
    uv_periodic_t scheduler;
    struct uv__queue node;
} iems_poll_task_t;

static void on_poll_complete(int status, const pipe_msg_t *result) {
    if (status == 0 && result && g_app.hub) {
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

static void on_poll_task_trigger(uv_periodic_t *handle) {
    iems_poll_task_t *task = (iems_poll_task_t *)handle->data;
    if (!g_assets_loaded) return;

    bool is_southbound = false;
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        if (g_connectivity_configs[i].id == task->bus_id &&
            strcmp(g_connectivity_configs[i].category, "SOUTHBOUND") == 0) {
            is_southbound = true;
            break;
        }
    }

    if (is_southbound) {
        if (g_bus_state[task->bus_id % MAX_VBUS] == BUS_STATE_CONNECTED) {
            edge_api_read_tag(task->bus_id, task->protocol, task->slave_id, task->tag_id,
                              on_poll_complete);
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
    task->interval_ms = 1000;
    for (uint32_t i = 0; i < g_tag_metadata_len; i++) {
        if (g_tag_metadata[i].id == tag_id && g_tag_metadata[i].poll_interval_ms > 0) {
            task->interval_ms = g_tag_metadata[i].poll_interval_ms;
            break;
        }
    }
    uv__queue_insert_tail(&g_poll_task_head, &task->node);
    uv_periodic_init(uv_default_loop(), &task->scheduler);
    task->scheduler.data = task;
    uv_periodic_set(&task->scheduler, 0, (double)task->interval_ms, NULL);
    uv_periodic_start(&task->scheduler, on_poll_task_trigger);
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
        if (sqlite3_step(stmt) == SQLITE_ROW &&
            strcmp((const char *)sqlite3_column_text(stmt, 0), "1") == 0)
            ready = true;
        sqlite3_finalize(stmt);
    }
    if (ready) {
        const char *sql =
            "SELECT t.bus_id, i.addr, t.tag_id, i.protocol FROM poll_tasks t INNER JOIN inventory "
            "i ON t.device_id = i.id;";
        char *err = NULL;
        if (sqlite3_exec(raw_db, sql, _load_tasks_cb, NULL, &err) == SQLITE_OK) {
            int count = 0;
            struct uv__queue *q;
            uv__queue_foreach(q, &g_poll_task_head) count++;
            if (count > 0) {
                g_assets_loaded = true;
                uv_timer_stop(handle);
                uv_close((uv_handle_t *)handle, (uv_close_cb)free);
                g_timer_load_tasks = NULL;
            }
        } else if (err)
            sqlite3_free(err);
    }
    sqlite3_close(raw_db);
}

typedef struct {
    int bus_id;
    int bus_idx;
} north_accept_ctx_t;

static void _on_north_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    north_accept_ctx_t *ctx = server->data;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int fd;
        uv_fileno((const uv_handle_t *)client, &fd);
        int new_fd = dup(fd);
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t *h = io_executor_create_from_fd(server->loop, new_fd, g_app.hub->io_pool);
            if (h) {
                bus_orchestrator_attach_io(g_app.orch, ctx->bus_id, h);
                if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
            } else
                close(new_fd);
        }
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

static void _try_connect_buses(uv_timer_t *handle) {
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        int bus_idx = c->id % MAX_VBUS;
        if (g_bus_state[bus_idx] != BUS_STATE_DISCONNECTED) continue;
        const char *path = edge_resolve_bus_path(c->id);
        if (!path) continue;
        if (strcmp(c->category, "SOUTHBOUND") == 0 && access(path, F_OK) == 0) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) continue;
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                io_handle_t *h = io_executor_create_from_fd(handle->loop, fd, g_app.hub->io_pool);
                if (h) {
                    bus_orchestrator_attach_io(g_app.orch, c->id, h);
                    if (h->ops && h->ops->enable_read) h->ops->enable_read(h, true);
                    g_bus_state[bus_idx] = BUS_STATE_CONNECTED;
                } else
                    close(fd);
            } else
                close(fd);
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
    g_adc_running = false;
    free(ctx);
}
static void _try_connect_internal_buses(uv_timer_t *handle) {
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        if (strcmp(c->category, "INTERNAL") != 0 || g_adc_running) continue;
        const char *path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                adc_thread_ctx_t *ctx = malloc(sizeof(adc_thread_ctx_t));
                ctx->fd = fd;
                ctx->hub = g_app.hub;
                g_adc_running = true;
                uv_thread_create(&g_adc_tid, _adc_thread_entry, ctx);
            } else
                close(fd);
        }
    }
}

static void _monitor_shadow_tick(uv_timer_t *handle) {
    (void)handle;
    if (g_app.hub && g_app.hub->shadow_table) {
        pipe_msg_t msg;
        if (datahub_read_shadow(g_app.hub, 1001, &msg) == 0) {
            printf("\033[36m[HEARTBEAT] Shadow Tag 1001 = %.2f\033[0m\n", msg.payload.val.f);
        }
    }
}

static void _on_task_close(uv_handle_t *handle) {
    uv_periodic_t *periodic = (uv_periodic_t *)handle->data;
    iems_poll_task_t *task = (iems_poll_task_t *)periodic->data;
    free(task);
}

static void iems_shutdown(uv_loop_t *loop) {
    if (g_timer_load_tasks) {
        uv_timer_stop(g_timer_load_tasks);
        uv_close((uv_handle_t *)g_timer_load_tasks, (uv_close_cb)free);
    }
    if (g_timer_vbus) {
        uv_timer_stop(g_timer_vbus);
        uv_close((uv_handle_t *)g_timer_vbus, NULL);
    }
    if (g_timer_internal) {
        uv_timer_stop(g_timer_internal);
        uv_close((uv_handle_t *)g_timer_internal, NULL);
    }
    while (!uv__queue_empty(&g_poll_task_head)) {
        struct uv__queue *q = uv__queue_head(&g_poll_task_head);
        iems_poll_task_t *task = uv__queue_data(q, iems_poll_task_t, node);
        uv__queue_remove(q);
        uv_periodic_stop(&task->scheduler);
        uv_close((uv_handle_t *)&task->scheduler.timer, _on_task_close);
    }
    uv_run(loop, UV_RUN_NOWAIT);
    if (g_adc_running) {
        g_adc_running = false;
        uv_thread_join(&g_adc_tid);
    }
    if (g_app.stream_eng) stream_engine_destroy(g_app.stream_eng);
    if (g_app.svc) service_delete(g_app.svc);
    if (g_app.db_service) sqlite_svc_destroy(g_app.db_service);
    if (g_app.orch) bus_orchestrator_destroy(g_app.orch);
    if (g_app.arb) arbiter_destroy(g_app.arb);
    if (g_app.drv_mgr) driver_manager_destroy(g_app.drv_mgr);
    if (g_app.hub) {
        datahub_destroy(g_app.hub);
        free(g_app.hub);
    }
    if (g_app.db_path) free(g_app.db_path);
    curl_global_cleanup();
}

static void on_signal_exit(uv_signal_t *handle, int signum) {
    uv_stop(handle->loop);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    curl_global_init(CURL_GLOBAL_ALL);
    uv_loop_t *loop = uv_default_loop();
    uv__queue_init(&g_poll_task_head);
    static uv_signal_t s1, s2;
    uv_signal_init(loop, &s1);
    uv_signal_start(&s1, on_signal_exit, SIGINT);
    uv_signal_init(loop, &s2);
    uv_signal_start(&s2, on_signal_exit, SIGTERM);
    g_app.arb = NULL;
    arbiter_init(&g_app.arb, loop);
    g_app.drv_mgr = driver_manager_create();
    modbus_driver_register(g_app.drv_mgr);
    const char *shm_name = getenv("EDGE_SHM_NAME") ? getenv("EDGE_SHM_NAME") : EDGE_SHM_BUS_NAME;
    g_app.hub = calloc(1, sizeof(datahub_t));
    datahub_init(g_app.hub, loop, shm_name, 4096, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, NULL);
    g_app.orch = bus_orchestrator_create(g_app.hub, g_app.arb, g_app.drv_mgr, 256, 256, 2048);
    g_app.hub->io_pool = bus_orchestrator_get_pool(g_app.orch);
    edge_api_init(g_app.orch, g_app.drv_mgr);
    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *conn = &g_connectivity_configs[i];
        const driver_contract_injection_t *inj = edge_model_find("ModbusRTU");
        if (strcmp(conn->category, "SOUTHBOUND") == 0) {
            if (inj)
                bus_orchestrator_setup_master(g_app.orch, conn->id, "ModbusRTU", (uint32_t)conn->id,
                                              inj);
        } else if (strcmp(conn->category, "NORTHBOUND") == 0) {
            if (inj) bus_orchestrator_setup_slave(g_app.orch, conn->id, "ModbusRTU", 1, inj);
        }
    }
    const char *db_env = getenv("EDGE_DB_PATH");
    g_app.db_path = strdup(db_env ? db_env : "iems.db");
    sqlite_config_t db_conf = {.db_path = g_app.db_path, .strategy = STRATEGY_LITE};
    g_app.db_service = sqlite_svc_create(loop, &db_conf);
    g_app.stream_eng =
        stream_engine_create(g_app.hub, NULL, g_app.orch, edge_stage_factory_get_registry());
    pipeline_loader_ctx_t loader_ctx = {.db = g_app.db_service, .orch = g_app.orch, .params = NULL};
    stream_engine_load_static_metadata(g_app.stream_eng, g_pipeline_metadata,
                                       g_pipeline_metadata_len, &loader_ctx);
    stream_engine_start(g_app.stream_eng);
    g_app.north = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(g_app.north, loop, &g_app.hub->ring, "IEMS_V4", g_app.db_service);
    g_app.svc = service_new(loop, g_app.hub, g_app.arb, g_app.db_service, g_app.stream_eng, NULL,
                            g_app.north);
    int rest_port = getenv("EDGE_REST_PORT") ? atoi(getenv("EDGE_REST_PORT")) : 9091;
    rest_server_create(loop, g_app.svc, rest_port);
    g_timer_load_tasks = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, g_timer_load_tasks);
    uv_timer_start(g_timer_load_tasks, iems_load_tasks_timer_cb, 50, 200);
    static uv_timer_t monitor_t;
    uv_timer_init(loop, &monitor_t);
    uv_timer_start(&monitor_t, _monitor_shadow_tick, 1000, 1000);
    static uv_timer_t vt;
    uv_timer_init(loop, &vt);
    g_timer_vbus = &vt;
    uv_timer_start(&vt, _try_connect_buses, 100, 1000);
    static uv_timer_t it;
    uv_timer_init(loop, &it);
    g_timer_internal = &it;
    uv_timer_start(&it, _try_connect_internal_buses, 100, 1000);

    for (int i = 0; i < g_connectivity_configs_len; i++) {
        const edge_connectivity_t *c = &g_connectivity_configs[i];
        if (strcmp(c->category, "NORTHBOUND") == 0) {
            const char *path = edge_resolve_bus_path(c->id);
            if (path) {
                uv_pipe_t *srv = malloc(sizeof(uv_pipe_t));
                uv_pipe_init(loop, srv, 0);
                unlink(path);
                if (uv_pipe_bind(srv, path) == 0 &&
                    uv_listen((uv_stream_t *)srv, 128, _on_north_connect) == 0) {
                    north_accept_ctx_t *ctx = malloc(sizeof(north_accept_ctx_t));
                    ctx->bus_id = c->id;
                    ctx->bus_idx = c->id % MAX_VBUS;
                    srv->data = ctx;
                    g_bus_state[ctx->bus_idx] = BUS_STATE_LISTENING;
                } else
                    uv_close((uv_handle_t *)srv, (uv_close_cb)free);
            }
        }
    }
    arbiter_resume(g_app.arb);
    service_run(g_app.svc);
    iems_shutdown(loop);
    return 0;
}
