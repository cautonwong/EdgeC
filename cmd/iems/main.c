#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "contracts.h"
#include "driver_manager.h"
#include "edge_params.h"
#include "edgeos.h"
#include "northbound_mgr.h"
#include "sqlite_service.h"
#include "req_driver_legacy.h"

// 外部驱动注册
extern int modbus_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;
static driver_manager_t *g_drv_mgr = NULL;

// [核心] 手动构造针对 IEMS 的契约注入包
static const driver_contract_injection_t iems_modbus_injection = {
    .lookup_by_key = (const void *(*)(uint32_t))edge_get_map_modbus,
    .lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_modbus_by_tag,
    .meta_list = NULL,
    .meta_list_len = 0};

// --- 1. 回调函数定义 ---

static void on_poll_complete(int status, const pipe_msg_t *result) {
    if (status != CMD_STATUS_SUCCESS || !result) return;
    
    float scale = 1.0f;
    // The result->tag_id is the physical tag_id, look up the logical one for publishing
    edge_tag_id_t tid = edge_lookup_modbus(result->tag_id, &scale);

    if (tid != 0) {
        pipe_msg_t msg;
        // The value from the driver is already scaled, but the contract gives us the final logical tag
        pipe_msg_init_scalar(&msg, tid, result->payload.val.f, uv_hrtime());
        datahub_publish(g_hub_ptr, &msg);
    }
}

static int _task_query_cb(void *user_data, int argc, char **argv, char **cols) {
    (void)cols;
    bus_orchestrator_t *orch = (bus_orchestrator_t *)user_data;
    if (argc < 4) return 0;

    int bus_id = atoi(argv[0]);
    uint32_t physical_reg = (uint32_t)atoi(argv[2]);
    const char *proto = argv[3];

    // Create the specific request object for the legacy driver path
    edge_req_t *req = req_legacy_driver_create(g_drv_mgr, proto, physical_reg, on_poll_complete);

    // Create the command message and attach the request
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;
    // The request's teardown function will handle all cleanup

    bus_orchestrator_submit_cmd(orch, bus_id, cmd);
    return 0;
}

static void iems_scheduler_tick(uv_periodic_t *handle) {
    (void)handle;
    sqlite_svc_t *db = service_get_storage(g_running_svc);
    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!raw_db) return;
    const char *sql =
        "SELECT t.bus_id, i.addr, t.tag_id, i.protocol "
        "FROM poll_tasks t JOIN inventory i ON t.device_id = i.id WHERE i.enabled = 1;";
    sqlite3_exec(raw_db, sql, _task_query_cb, g_orch_ptr, NULL);
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        free(req);
        return;
    }
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    io_handle_t *h = io_executor_create_from_fd(req->handle->loop, fd, g_hub_ptr->io_pool);
    bus_orchestrator_attach_io(g_orch_ptr, 0, h);
    h->ops->enable_read(h, true);
    printf(" 🔗 [Omnis] VBUS Online.\n");
    free(req);
}

static void _try_vbus_connect(uv_timer_t *handle) {
    char *path = getenv("OMNIS_VBUS_PATH") ?: "/tmp/vbus0.sock";
    if (access(path, F_OK) != 0) return;
    uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(handle->loop, pipe, 0);
    uv_connect_t *req = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(req, pipe, path, on_vbus_connect);
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

static void on_boot_delayed(uv_timer_t *handle) {
    service_t *svc = (service_t *)handle->data;
    static uv_periodic_t sched_h;
    uv_periodic_init(handle->loop, &sched_h);
    uv_periodic_set(&sched_h, 0, 1000.0, NULL);
    uv_periodic_start(&sched_h, iems_scheduler_tick);
    arbiter_resume(service_get_arbiter(svc));
    printf(" 🚀 [IEMS] Omnis Runner Active.\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    printf(
        "\n--- EdgeOS IEMS (Omnis Sprint 4) ---\
");

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    g_drv_mgr = driver_manager_create();
    modbus_driver_register(g_drv_mgr);
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "iems", 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;

    bus_orchestrator_t *io_orch = bus_orchestrator_create(hub, arbiter, g_drv_mgr, 8, 32, 2048);
    g_orch_ptr = io_orch;
    hub->io_pool = bus_orchestrator_get_pool(io_orch);

    // 使用本地定义的注入包
    bus_orchestrator_setup_master(io_orch, 0, "ModbusRTU", 1, &iems_modbus_injection);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);

    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(db_service);
    if (raw_db) {
        sqlite3_exec(raw_db, "DELETE FROM inventory; DELETE FROM poll_tasks;", NULL, NULL, NULL);
        sqlite3_exec(raw_db, "INSERT INTO inventory VALUES ('M1', 'Meter', 1, 'ModbusRTU', 1);",
                     NULL, NULL, NULL);
        sqlite3_exec(raw_db,
                     "INSERT INTO poll_tasks (device_id, tag_id, interval_ms, bus_id) VALUES "
                     "('M1', 0, 1000, 0);",
                     NULL, NULL, NULL);
    }

    stream_engine_t *se = stream_engine_create(hub, NULL);
    northbound_mgr_t *north = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north, loop, &hub->ring, "iems", db_service);
    service_t *svc = service_new(loop, hub, arbiter, db_service, se, NULL, north);
    g_running_svc = svc;
    north->user_data = svc;

    unlink("/tmp/iems_ui.sock");
    uv_pipe_t *ui_srv = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, ui_srv, 0);
    uv_pipe_bind(ui_srv, "/tmp/iems_ui.sock");
    uv_listen((uv_stream_t *)ui_srv, 10, on_ui_connect);

    uv_timer_t *vt = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, vt);
    uv_timer_start(vt, _try_vbus_connect, 100, 1000);

    static uv_timer_t boot_t;
    uv_timer_init(loop, &boot_t);
    boot_t.data = svc;
    uv_timer_start(&boot_t, on_boot_delayed, 500, 0);

    return service_run(svc);
}