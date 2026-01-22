#include "contracts.h"
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "edge_api.h"
#include "js_engine.h"
#include "northbound_mgr.h"
#include "queue.h"
#include "sched_time.h"
#include "sqlite_service.h"

extern int modbus_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;
static driver_manager_t *g_drv_mgr = NULL;
static uv_timer_t *g_vbus_timer = NULL;

typedef struct {
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    char protocol[16];
    uint32_t interval_ms;
    uint64_t last_run_ms;
    struct uv__queue node;
} iems_poll_task_t;

static struct uv__queue g_poll_task_head;

static const driver_contract_injection_t iems_modbus_injection = {
    .lookup_by_key = (const void *(*)(uint32_t))edge_get_map_modbus,
    .lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_modbus_by_tag,
    .meta_list = NULL,
    .meta_list_len = 0};

static void on_poll_complete(int status, const pipe_msg_t *result) {
    if (status != 0 || !result) return;

    // The result->tag_id from the legacy request is the PHYSICAL tag (e.g. modbus register).
    // We must look up the LOGICAL tag id to publish to the rest of the system.
    float scale = 1.0f;
    edge_tag_id_t logical_tid = edge_lookup_modbus(result->tag_id, &scale);

    if (logical_tid != 0) {
        pipe_msg_t msg_to_publish;
        // The driver already applied its own scaling, so we just use the final value.
        // We create a new message with the correct LOGICAL tag id.
        pipe_msg_init_scalar(&msg_to_publish, logical_tid, result->payload.val.f, uv_hrtime());
        datahub_publish(g_hub_ptr, &msg_to_publish);
    }
}

static void iems_scheduler_tick(uv_periodic_t *handle) {
    uint64_t now = uv_now(handle->timer.loop);
    struct uv__queue *q;
    static int tick_count = 0;
    if (++tick_count % 10 == 0) {
        printf(" 💓 [Scheduler] Alive at %lu\n", now);
        fflush(stdout);
    }

    uv__queue_foreach(q, &g_poll_task_head) {
        iems_poll_task_t *task = uv__queue_data(q, iems_poll_task_t, node);
        if (now - task->last_run_ms >= (uint64_t)task->interval_ms) {
            edge_api_read_tag(task->bus_id, task->protocol, task->slave_id, task->tag_id,
                              (void *)edge_get_map_modbus_by_tag, (void *)edge_get_map_modbus,
                              on_poll_complete);
            task->last_run_ms = now;
        }
    }
}

static int _load_tasks_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)user_data;
    (void)cols;
    if (argc < 5) return 0;
    iems_poll_task_t *task = calloc(1, sizeof(iems_poll_task_t));
    task->bus_id = atoi(argv[0]);
    task->slave_id = (uint32_t)atoi(argv[1]);
    task->tag_id = (uint32_t)atoi(argv[2]);
    strncpy(task->protocol, argv[3], 15);
    task->interval_ms = (uint32_t)atoi(argv[4]);
    uv__queue_insert_tail(&g_poll_task_head, &task->node);
    return 0;
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        free(req);
        return;
    }
    if (g_vbus_timer) uv_timer_stop(g_vbus_timer);
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    io_handle_t *h = io_executor_create_from_fd(req->handle->loop, fd, g_hub_ptr->io_pool);
    bus_orchestrator_attach_io(g_orch_ptr, 0, h);
    h->ops->enable_read(h, true);
    printf(" ✅ [VBUS] Physical line connected to Bus 0.\n");
    fflush(stdout);
    free(req);
}

static void _try_vbus_connect(uv_timer_t *handle) {
    char *path = getenv("OMNIS_VBUS_PATH") ?: EDGE_VBUS_PATH;
    if (access(path, F_OK) != 0) return;
    uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(handle->loop, pipe, 0);
    uv_connect_t *req = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(req, pipe, path, on_vbus_connect);
}

static int _check_db_empty_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)cols;
    if (argc > 0 && argv[0]) {
        int *count = (int*)user_data;
        *count = atoi(argv[0]);
    }
    return 0;
}

static void on_boot_delayed(uv_timer_t *handle) {
    service_t *svc = (service_t *)handle->data;
    sqlite_svc_t *db = service_get_storage(svc);
    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(db);

    if (!raw_db) {
        uv_timer_start(handle, on_boot_delayed, 500, 0);
        return;
    }

    const char *schema =
        "CREATE TABLE IF NOT EXISTS inventory (id TEXT PRIMARY KEY, type TEXT, addr INT, protocol "
        "TEXT, enabled INT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (id INTEGER PRIMARY KEY, device_id TEXT, tag_id "
        "INT, interval_ms INT, bus_id INT);";

    char *err = NULL;
    sqlite3_exec(raw_db, schema, NULL, NULL, &err);
    if (err) {
        printf(" ❌ [DB] Schema Error: %s\n", err);
        sqlite3_free(err);
    }

    // [SYS-05] Check if DB is empty and provision with defaults if so
    int inventory_count = 0;
    sqlite3_exec(raw_db, "SELECT COUNT(*) FROM inventory;", _check_db_empty_cb, &inventory_count, &err);
    if (inventory_count == 0) {
        printf(" ℹ️ [DB] Empty inventory detected, provisioning with factory defaults...\n");
        char sql_buf[256];
        for (int i = 0; i < g_default_hardware_topology_len; i++) {
            const edge_device_t *dev = &g_default_hardware_topology[i];
            snprintf(sql_buf, sizeof(sql_buf), 
                     "INSERT INTO inventory (id, type, addr, protocol, enabled) VALUES ('DEV_%d_%s_%d', 'default', %d, '%s', 1);",
                     dev->bus_id, dev->protocol_name, dev->address, dev->address, dev->protocol_name);
            sqlite3_exec(raw_db, sql_buf, NULL, NULL, &err);
            if (err) {
                 printf(" ❌ [DB] Default insert error: %s\n", err);
                 sqlite3_free(err);
                 err = NULL;
            }
        }
    }
    
    // Load tasks from the (now populated) database into the in-memory scheduler
    uv__queue_init(&g_poll_task_head);
    const char *sql =
        "SELECT t.bus_id, i.addr, t.tag_id, i.protocol, t.interval_ms FROM poll_tasks t "
        "JOIN inventory i ON t.device_id = i.id WHERE i.enabled = 1;";
    sqlite3_exec(raw_db, sql, _load_tasks_cb, NULL, NULL);

    int count = 0;
    struct uv__queue *q;
    uv__queue_foreach(q, &g_poll_task_head) count++;
    printf(" ✅ [IEMS] Boot: Memory Scheduler Ready. Tasks Active: %d\n", count);

    static uv_periodic_t sched_h;
    uv_periodic_init(handle->loop, &sched_h);
    uv_periodic_set(&sched_h, 0, 100.0, NULL);
    uv_periodic_start(&sched_h, iems_scheduler_tick);

    bus_orchestrator_run(g_orch_ptr);
    arbiter_resume(service_get_arbiter(svc));
    printf(" 🚀 [IEMS] ONLINE. Dispatching packets.\n");
    fflush(stdout);
}

static void on_ui_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int cfd;
        uv_fileno((const uv_handle_t *)client, &cfd);
        edge_fd_send(cfd, shmring_get_fd(&g_hub_ptr->ring));
        edge_fd_send(cfd, g_hub_ptr->shadow_fd);
    }
    uv_close((uv_handle_t *)client, (uv_close_cb)free);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    printf(
        "\n--- EdgeOS IEMS (Omnis Stable v4.6) ---\
");
    fflush(stdout);

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    g_drv_mgr = driver_manager_create();
    modbus_driver_register(g_drv_mgr);
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, EDGE_SHM_BUS_NAME, 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;

    g_orch_ptr = bus_orchestrator_create(hub, arbiter, g_drv_mgr, 8, 32, 2048);
    hub->io_pool = bus_orchestrator_get_pool(g_orch_ptr);
    edge_api_init(g_orch_ptr, g_drv_mgr);
    // [修正] 将总线 0 的 device_id 设为 0，与 BusID 严格一致
    bus_orchestrator_setup_master(g_orch_ptr, 0, "ModbusRTU", 0, &iems_modbus_injection);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);

    stream_engine_t *se = stream_engine_create(hub, NULL);
    service_t *svc = service_new(loop, hub, arbiter, db_service, se, NULL, NULL);
    g_running_svc = svc;

    // Load pipelines from JSON config now that all services are available
    js_engine_t *js_engine = js_engine_create(svc, 1024 * 1024);
    pipeline_loader_ctx_t loader_ctx = {
        .db = db_service,
        .js_engine = js_engine,
        .orch = g_orch_ptr,
        .params = NULL
    };
    stream_engine_load_config(se, "config/iems_pipelines.json", &loader_ctx);

    unlink(EDGE_UI_SOCKET_PATH);
    uv_pipe_t *ui_srv = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ui_srv, 0);
    uv_pipe_bind(ui_srv, EDGE_UI_SOCKET_PATH);
    uv_listen((uv_stream_t *)ui_srv, 10, on_ui_connect);

    g_vbus_timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, g_vbus_timer);
    uv_timer_start(g_vbus_timer, _try_vbus_connect, 100, 1000);

    static uv_timer_t boot_t;
    uv_timer_init(loop, &boot_t);
    boot_t.data = svc;
    uv_timer_start(&boot_t, on_boot_delayed, 800, 0);
    return service_run(svc);
}
