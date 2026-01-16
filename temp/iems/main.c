#include <fcntl.h>
#include <signal.h>
#include <sqlite3.h>
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
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "edge_req.h"
#include "rest_server.h"

extern int modbus_driver_register(driver_manager_t *mgr);
extern int dlms_driver_register(driver_manager_t *mgr);

static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;

typedef struct {
    uint32_t tag_id;
    int interval_ms;
    int bus_id;
    calendar_task_t sched_task;
    uv_edge_tag_req_t req; 
} poll_task_entry_t;

static poll_task_entry_t g_active_tasks[256];
static int g_task_count = 0;

static const void* _smart_lookup_by_tag(uint32_t tag_id) {
    return edge_get_map_modbus_by_tag(tag_id);
}

static void on_read_complete(uv_edge_tag_req_t *req, int status, const pipe_msg_t *val) {
    (void)val;
    if (status != 0) {
        LOG_WARN(LOG_CAT_BUS, "[SCHED] Poll FAILED for Tag %u, Status %d", req->tag_id, status);
    }
}

static void on_task_fire(calendar_task_t *task) {
    poll_task_entry_t *entry = (poll_task_entry_t *)task->user_data;
    entry->req.bus_id = entry->bus_id;
    edge_tag_read(task->loop, &entry->req, on_read_complete, entry->tag_id);
}

static int load_poll_tasks(sqlite_svc_t *db_svc, uv_loop_t *loop) {
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(db_svc);
    if (!db) return -1;
    const char *sql = "SELECT t.tag_id, t.interval_ms, t.bus_id, i.addr FROM poll_tasks t JOIN inventory i ON t.device_id = i.id WHERE i.enabled = 1";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    g_task_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && g_task_count < 256) {
        poll_task_entry_t *entry = &g_active_tasks[g_task_count++];
        entry->tag_id = (uint32_t)sqlite3_column_int(stmt, 0);
        entry->interval_ms = sqlite3_column_int(stmt, 1);
        entry->bus_id = sqlite3_column_int(stmt, 2);
        entry->req.slave_id = (uint8_t)sqlite3_column_int(stmt, 3);
        
        memset(&entry->sched_task, 0, sizeof(calendar_task_t));
        entry->sched_task.type = CAL_TASK_PERIODIC_READ;
        entry->sched_task.period_sec = (uint32_t)(entry->interval_ms / 1000);
        if (entry->sched_task.period_sec == 0) entry->sched_task.period_sec = 1;
        entry->sched_task.on_fire = on_task_fire;
        entry->sched_task.user_data = entry;
        entry->sched_task.loop = loop;
        calendar_schedule(&entry->sched_task);
    }
    sqlite3_finalize(stmt);
    return g_task_count;
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "[BOOT][Main] VBus Connection Failed: %s\n", uv_strerror(status));
        free(req); return;
    }
    uv_stream_t *stream = req->handle;
    int fd;
    uv_fileno((const uv_handle_t *)stream, &fd);
    io_handle_t *h = io_executor_create_from_fd(stream->loop, fd, g_hub_ptr->io_pool);
    bus_orchestrator_attach_io(g_orch_ptr, 0, h);
    h->ops->enable_read(h, true);
    fprintf(stderr, "[BOOT][Main] VBus connected and attached.\n");
    free(req);
}

static void _on_vbus_connect_timer(uv_timer_t *handle) {
    uv_loop_t *loop = handle->loop;
    char *vbus_path = getenv("OMNIS_VBUS_PATH") ? getenv("OMNIS_VBUS_PATH") : "/tmp/vbus0.sock";
    uv_pipe_t *vbus_client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, vbus_client, 0);
    uv_connect_t *connect_req = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(connect_req, vbus_client, vbus_path, on_vbus_connect);
    uv_timer_stop(handle);
    uv_close((uv_handle_t*)handle, (uv_close_cb)free);
}

static void _loop_keepalive(uv_timer_t *h) { (void)h; }

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);
    uv_timer_t keepalive;
    uv_timer_init(loop, &keepalive);
    uv_timer_start(&keepalive, _loop_keepalive, 1000, 1000);

    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);
    dlms_driver_register(drv_mgr);
    
    driver_contract_injection_t inject = {
        .lookup_by_tag = _smart_lookup_by_tag,
        .meta_list = (const void *const *)g_tag_metadata,
        .meta_list_len = g_tag_metadata_len
    };

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "iems_v4.5", 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;
    
    bus_orchestrator_t *io_orch = bus_orchestrator_create(hub, arb, drv_mgr, 8, 128, 2048);
    g_orch_ptr = io_orch;
    hub->io_pool = bus_orchestrator_get_pool(io_orch);
    bus_orchestrator_setup_master(io_orch, 0, "ModbusRTU", 0, &inject);

    uv_timer_t *conn_timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, conn_timer);
    uv_timer_start(conn_timer, _on_vbus_connect_timer, 100, 0);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    load_poll_tasks(db_service, loop);
    
    stream_engine_t *stream_engine = stream_engine_create(hub, NULL);
    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(stream_engine, g_pipeline_metadata, g_pipeline_metadata_len, &pl_ctx);
    
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub->ring, "iems", db_service);
    
    edge_params_t *param_mgr = edge_params_create(db_service, hub);
    edge_params_register_metadata(param_mgr, g_param_metadata, g_param_metadata_len);
    
    service_t *svc = service_new(loop, hub, arb, db_service, stream_engine, param_mgr, north_mgr,
                                 edge_lookup_modbus, edge_lookup_dlt645, 
                                 (void*)edge_get_map_modbus_by_tag, (void*)edge_get_map_dlt645_by_tag);
    north_mgr->user_data = svc;
    rest_server_t *rest = rest_server_create(loop, svc, getenv("EDGE_REST_PORT") ? atoi(getenv("EDGE_REST_PORT")) : 9091);
    (void)rest;

    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);
    
    service_delete(svc);
    datahub_destroy(hub);
    free(hub);
    edge_log_destroy(logger);
    return exit_code;
}