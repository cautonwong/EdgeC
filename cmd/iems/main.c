#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "sqlite_service.h"
#include "stream_engine.h"

extern int dlt645_driver_register(driver_manager_t *mgr);
extern int dlms_driver_register(driver_manager_t *mgr);
extern int modbus_driver_register(driver_manager_t *mgr);
extern int iec104_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub = NULL;
static bus_orchestrator_t *g_io_orch = NULL;

static void on_new_sim_conn(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int fd;
        uv_fileno((const uv_handle_t *)client, &fd);
        io_handle_t *h = io_executor_create_from_fd(server->loop, fd, g_hub->io_pool);
        bus_orchestrator_attach_io(g_io_orch, 0, h);
    } else {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
    }
}

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    dlt645_driver_register(drv_mgr);
    dlms_driver_register(drv_mgr);
    modbus_driver_register(drv_mgr);
    iec104_driver_register(drv_mgr);

    datahub_t hub;
    datahub_init(&hub, loop, "iems_v4.5", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub = &hub;

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    g_io_orch = io_orch;

    uv_pipe_t vbus_server;
    uv_pipe_init(loop, &vbus_server, 0);
    const char *vbus_path = "/tmp/vbus0.sock";
    unlink(vbus_path);
    uv_pipe_bind(&vbus_server, vbus_path);
    uv_listen((uv_stream_t *)&vbus_server, 128, on_new_sim_conn);

    sqlite_config_t db_conf = {
        .db_path = "iems.db", .strategy = STRATEGY_LITE, .max_db_size_mb = 100};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);

    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(stream_engine, g_pipeline_metadata, g_pipeline_metadata_len,
                                       &pl_ctx);

    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "iems", db_service);
    north_mgr->rest_port = 8081;
    north_mgr->web_port = 8080;
    strncpy(north_mgr->web_root, "./web/dist", 255);

    service_t *svc = service_new(loop, &hub, arbiter, db_service, stream_engine, NULL, north_mgr);
    g_running_svc = svc;

    // 核心修复：在启动线程前，完成所有依赖注入
    north_mgr->user_data = svc;
    northbound_mgr_start(north_mgr);

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    service_delete(svc);
    northbound_mgr_stop(north_mgr);
    free(north_mgr);
    bus_orchestrator_destroy(io_orch);
    stream_engine_destroy(stream_engine);
    driver_manager_destroy(drv_mgr);
    arbiter_destroy(arbiter);
    sqlite_svc_destroy(db_service);
    datahub_destroy(&hub);
    edge_log_destroy(logger);
    return exit_code;
}
