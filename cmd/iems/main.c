#include <fcntl.h>
#include <signal.h>
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
#include "sqlite_service.h"
#include "stream_engine.h"

extern int modbus_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;
static driver_contract_injection_t g_modbus_inject;

// 使用合法的 Arbiter 总线 ID (匹配 Reflex 默认 Bus 0)
#define TEST_BUS_ID 0

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

static void on_poll_timer(uv_timer_t *handle) {
    (void)handle;
    if (!g_orch_ptr) return;
    cmd_msg_t cmd = {0};
    cmd_msg_init_read(&cmd, 1001, ARBITER_PRIO_NORMAL);
    strncpy(cmd.protocol, "ModbusRTU", 15);
    snprintf(cmd.req_id, sizeof(cmd.req_id), "POLL_%lu", (unsigned long)time(NULL));

    int rc = bus_orchestrator_submit_cmd(g_orch_ptr, TEST_BUS_ID, &cmd);
    if (rc != 0) {
        fprintf(stderr, "[ERROR][IEMS] Failed to submit POLL: %d\n", rc);
    }
}

static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        int fd;
        uv_fileno((const uv_handle_t *)client, &fd);
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        fprintf(stderr, "[OMNIS] Mimic connected on FD %d\n", fd);
        io_handle_t *h = io_executor_create_from_fd(server->loop, fd, g_hub_ptr->io_pool);
        bus_orchestrator_attach_io(g_orch_ptr, TEST_BUS_ID, h);
        h->ops->enable_read(h, true);
    } else {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);

    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);

    g_modbus_inject.lookup_by_key = (void *)edge_get_map_modbus;
    g_modbus_inject.lookup_by_tag = (void *)edge_get_map_modbus_by_tag;

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "iems_v4.5", 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;

    bus_orchestrator_t *io_orch = bus_orchestrator_create(hub, arbiter, drv_mgr, 8, 32, 2048);
    hub->io_pool = bus_orchestrator_get_pool(io_orch);
    g_orch_ptr = io_orch;

    bus_orchestrator_setup_master(io_orch, TEST_BUS_ID, "ModbusRTU", TEST_BUS_ID, &g_modbus_inject);

    uv_pipe_t *vbus_server = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, vbus_server, 0);
    unlink("/tmp/vbus0.sock");
    uv_pipe_bind(vbus_server, "/tmp/vbus0.sock");
    uv_listen((uv_stream_t *)vbus_server, 128, on_new_connection);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(hub, NULL);

    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(stream_engine, g_pipeline_metadata, g_pipeline_metadata_len,
                                       &pl_ctx);

    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub->ring, "iems", db_service);

    char *rest_port_env = getenv("EDGE_REST_PORT");
    north_mgr->rest_port = rest_port_env ? atoi(rest_port_env) : 9091;

    service_t *svc = service_new(loop, hub, arbiter, db_service, stream_engine, NULL, north_mgr);
    g_running_svc = svc;
    north_mgr->user_data = svc;

    uv_timer_t *poll_timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, poll_timer);
    uv_timer_start(poll_timer, on_poll_timer, 100, 500);

    uv_async_t *keepalive = malloc(sizeof(uv_async_t));
    uv_async_init(loop, keepalive, NULL);

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);

    fprintf(stderr, "[OMNIS] System Ready on TEST BUS %d. Main Loop Starting.\n", TEST_BUS_ID);
    int exit_code = service_run(svc);

    fprintf(stderr, "[OMNIS] System loop returned with %d. Cleaning up...\n", exit_code);

    northbound_mgr_stop(north_mgr);
    bus_orchestrator_destroy(io_orch);
    service_delete(svc);
    datahub_destroy(hub);

    free(hub);
    free(north_mgr);
    edge_log_destroy(logger);
    return exit_code;
}