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

static service_t *g_svc = NULL;
static void on_sig(int s) {
    (void)s;
    if (g_svc) service_stop(g_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_t *logger = edge_log_create(NULL, true);
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "evse", 1024);
    sqlite_config_t db_conf = {.db_path = "evse.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_conf);

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);
    driver_manager_t *drv = driver_manager_create();
    stream_engine_t *eng = stream_engine_create(&hub, NULL);
    edge_params_t *p = edge_params_create(db);
    edge_params_register_metadata(p, g_param_metadata, g_param_metadata_len);

    bus_orchestrator_t *orch = bus_orchestrator_create(&hub, arb, drv, 8, 16, 2048);
    g_svc = service_new(loop, &hub, arb, db, eng, p, NULL);

    signal(SIGTERM, on_sig);
    bus_orchestrator_run(orch);
    edge_log_out(logger, LOG_CAT_CORE, 0, "EVSE Started");

    int r = service_run(g_svc);

    service_delete(g_svc);
    bus_orchestrator_destroy(orch);
    stream_engine_destroy(eng);
    driver_manager_destroy(drv);
    arbiter_destroy(arb);
    edge_params_destroy(p);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);
    edge_log_destroy(logger);
    return r;
}