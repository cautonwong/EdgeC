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
#include "js_engine.h"
#include "northbound_mgr.h"
#include "queue.h"
#include "sched_time.h"
#include "sqlite_service.h"

extern int modbus_driver_register(driver_manager_t *mgr);

static datahub_t *g_hub = NULL;
static bus_orchestrator_t *g_orch = NULL;
static stream_engine_t *g_se = NULL;
static arbiter_t *g_arbiter = NULL;
static bool g_bus_online[8] = {false};

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    char protocol[16];
    uint32_t interval_ms;
    uint64_t last_run_ms;
    struct uv__queue node;
} omnis_poll_task_t;

static struct uv__queue g_tasks;

static void on_poll_complete(int status, const pipe_msg_t *result) {
    if (status == 0 && result) {
        datahub_publish(g_hub, result);
    }
}

static void omnis_scheduler_tick(uv_periodic_t *handle) {
    uint64_t now = uv_now(handle->timer.loop);
    struct uv__queue *q;
    uv__queue_foreach(q, &g_tasks) {
        omnis_poll_task_t *t = uv__queue_data(q, omnis_poll_task_t, node);
        if (g_bus_online[t->bus_id % 8] && (now - t->last_run_ms >= (uint64_t)t->interval_ms)) {
            edge_api_read_tag(t->bus_id, t->protocol, t->slave_id, t->tag_id,
                              (void *)edge_get_map_modbus_by_tag, (void *)edge_get_map_modbus,
                              on_poll_complete);
            t->last_run_ms = now;
        }
    }
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    int bus_id = (int)(intptr_t)req->data;
    if (status < 0) {
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        free(req);
        return;
    }
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    io_handle_t *h = io_executor_create_from_fd(req->handle->loop, fd, g_hub->io_pool);
    bus_orchestrator_attach_io(g_orch, bus_id, h);
    h->ops->enable_read(h, true);
    g_bus_online[bus_id % 8] = true;
    printf(" 💥 [Omnis] Bus %d CONNECTED! Data flowing...\n", bus_id);
    fflush(stdout);
    free(req);
}

static void _try_connect_buses(uv_timer_t *handle) {
    static int retry_count = 0;
    for (int i = 0; i < g_hw_resources_len; i++) {
        const hw_resource_info_t *hw = &g_hw_resources[i];
        if (g_bus_online[hw->id % 8]) continue;

        char *path = getenv("OMNIS_VBUS_PATH");
        if (!path) path = "/tmp/vbus0.sock";

        printf(" 📡 [Omnis] Probing Matrix at: [%s]\n", path);
        if (access(path, F_OK) != 0) {
            if (++retry_count % 10 == 0) {
                printf(" ⏳ [Bus %d] Path [%s] not found on disk yet. (Matrix warming up?)\n",
                       hw->id, path);
                fflush(stdout);
            }
            continue;
        }

        printf(" 📡 [Bus %d] Found Matrix at '%s'. Connecting...\n", hw->id, path);
        fflush(stdout);

        uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
        uv_pipe_init(handle->loop, pipe, 0);
        uv_connect_t *req = malloc(sizeof(uv_connect_t));
        req->data = (void *)(intptr_t)hw->id;
        uv_pipe_connect(req, pipe, path, on_vbus_connect);
    }
}

static void on_ui_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int cfd;
        uv_fileno((const uv_handle_t *)client, &cfd);
        edge_fd_send(cfd, shmring_get_fd(&g_hub->ring));
        edge_fd_send(cfd, g_hub->shadow_fd);
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

int main(int argc, char **argv) {
    uv_loop_t *loop = uv_default_loop();
    printf("\n--- EdgeOS Omnis Core (Trace V4.8) ---\n");
    fflush(stdout);

    arbiter_init(&g_arbiter, loop);
    g_hub = calloc(1, sizeof(datahub_t));
    datahub_init(g_hub, loop, EDGE_SHM_BUS_NAME, 1024, 1024, g_tag_metadata, g_tag_metadata_len,
                 NULL, edge_tag_get_compact_index, NULL);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_svc = sqlite_svc_create(loop, &db_conf);
    js_engine_t *js_eng = js_engine_create(NULL, 1024 * 1024);

    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);
    g_orch = bus_orchestrator_create(g_hub, g_arbiter, drv_mgr, 8, 32, 2048);
    g_hub->io_pool = bus_orchestrator_get_pool(g_orch);
    edge_api_init(g_orch, drv_mgr);

    printf("\n🔎 [Asset Audit] Contract DNA discovered:\n");
    uv__queue_init(&g_tasks);
    for (int i = 0; i < g_hw_resources_len; i++) {
        const hw_resource_info_t *hw = &g_hw_resources[i];
        printf(" ├── Bus[%d]: %s\n", hw->id, hw->label);
        bus_orchestrator_setup_master(g_orch, hw->id, "ModbusRTU", (uint32_t)hw->id, NULL);
        for (int j = 0; j < g_default_hardware_topology_len; j++) {
            const edge_device_t *dev = &g_default_hardware_topology[j];
            if (dev->bus_id != hw->id) continue;
            printf(" │   └── Dev:%u [%s]\n", dev->address, dev->protocol_name);
            for (uint32_t k = 0; k < g_tag_metadata_len; k++) {
                if (g_tag_metadata[k].poll_interval_ms > 0) {
                    omnis_poll_task_t *t = calloc(1, sizeof(omnis_poll_task_t));
                    t->bus_id = dev->bus_id;
                    t->slave_id = dev->address;
                    t->tag_id = g_tag_metadata[k].id;
                    t->interval_ms = g_tag_metadata[k].poll_interval_ms;
                    strncpy(t->protocol, dev->protocol_name, 15);
                    uv__queue_insert_tail(&g_tasks, &t->node);
                    printf(" │       • TagID %u (%ums)\n", t->tag_id, t->interval_ms);
                }
            }
        }
    }
    printf("🔎 [Audit Summary] All tasks scheduled.\n\n");
    fflush(stdout);

    g_se = stream_engine_create(g_hub, NULL);
    pipeline_loader_ctx_t lctx = {.db = db_svc, .js_engine = js_eng, .orch = g_orch};
    stream_engine_load_static_metadata(g_se, g_pipeline_metadata, g_pipeline_metadata_len, &lctx);

    unlink(EDGE_UI_SOCKET_PATH);
    uv_pipe_t *ui_srv = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ui_srv, 0);
    if (uv_pipe_bind(ui_srv, EDGE_UI_SOCKET_PATH) == 0)
        uv_listen((uv_stream_t *)ui_srv, 10, on_ui_connect);

    static uv_timer_t conn_timer;
    uv_timer_init(loop, &conn_timer);

    uv_timer_start(&conn_timer, _try_connect_buses, 100, 500);  // 100ms 启动，500ms 探测频率

    static uv_periodic_t sched_h;
    uv_periodic_init(loop, &sched_h);
    uv_periodic_set(&sched_h, 0, 100.0, NULL);
    uv_periodic_start(&sched_h, omnis_scheduler_tick);

    bus_orchestrator_run(g_orch);
    arbiter_resume(g_arbiter);
    printf(" 🚀 [OMNIS] SYSTEM ONLINE. Heartbeat pulsing.\n");
    fflush(stdout);
    return uv_run(loop, UV_RUN_DEFAULT);
}
