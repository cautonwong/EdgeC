#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;
static void handle_sigpwr(int sig) {
    (void)sig;
    if (g_svc) service_stop(g_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_t *logger = edge_log_create(NULL, true);
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "vpp_demo", 1024, g_tag_metadata_len, edge_tag_get_compact_index);

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);

    sqlite_config_t db_conf = {.db_path = "vpp.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_conf);

    driver_manager_t *drv_mgr = driver_manager_create();
    edge_params_t *params = edge_params_create(db);
    stream_engine_t *engine = stream_engine_create(&hub, NULL);

    g_svc = service_new(loop, &hub, arb, db, engine, params, NULL);

    signal(SIGTERM, handle_sigpwr);

    edge_log_out(logger, LOG_CAT_CORE, 0, "VPP Demo Running...");
    int ret = service_run(g_svc);

    service_delete(g_svc);
    stream_engine_destroy(engine);
    edge_params_destroy(params);
    driver_manager_destroy(drv_mgr);
    sqlite_svc_destroy(db);
    arbiter_destroy(arb);
    datahub_destroy(&hub);
    edge_log_destroy(logger);

    return ret;
}
