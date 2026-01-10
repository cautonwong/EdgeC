/**
 * @file main.c
 * @brief IEMS Controller - Full Stack Simulation (Phase 13 Final)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "stream_engine.h"

extern void smoke_stimulator_start(datahub_t *hub);
extern int modbus_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;
static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

int main(int argc, char **argv) {
    bool smoke_mode = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--smoke-test") == 0) smoke_mode = true;
    }

    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);

    datahub_t hub;
    datahub_init(&hub, loop, "iems_omnis", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);

    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(stream_engine, g_pipeline_metadata, g_pipeline_metadata_len,
                                       &pl_ctx);

    service_t *svc = service_new(loop, &hub, arbiter, db_service, stream_engine, NULL, NULL);
    g_running_svc = svc;

    if (smoke_mode) {
        LOG_WARN(LOG_CAT_CORE,
                 "SMOKE TEST: Verifying Reflex Arc (Voltage > 260V -> Trigger Tag 8001)");
        smoke_stimulator_start(&hub);

        // 增加到 8秒，确保数据注入完整
        uv_timer_t *t = malloc(sizeof(uv_timer_t));
        uv_timer_init(loop, t);
        uv_timer_start(t, (uv_timer_cb)on_sigterm, 8000, 0);
    }

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);

    int exit_code = service_run(svc);

    service_delete(svc);
    bus_orchestrator_destroy(io_orch);
    stream_engine_destroy(stream_engine);
    driver_manager_destroy(drv_mgr);
    arbiter_destroy(arbiter);
    sqlite_svc_destroy(db_service);
    datahub_destroy(&hub);
    edge_log_destroy(logger);

    return exit_code;
}