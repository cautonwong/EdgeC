/**
 * @file main.c
 * @brief VPP Demo Entry Point (Refined)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;

static void handle_shutdown(int sig) {
    (void)sig;
    if (g_svc) {
        LOG_INFO(LOG_CAT_SCHED, "VPP Demo Shutdown Triggered.");
        service_stop(g_svc);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_init(NULL, false);

    uv_loop_t *loop = uv_default_loop();

    // 1. Foundation
    datahub_t hub;
    datahub_init(&hub, loop, "vpp_demo_shm", 512);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);

    // 2. Control Plane
    service_options_t opts = {.mode = "VPP_DEMO"};
    g_svc = service_new(&opts);
    service_set_datahub(g_svc, &hub);
    service_set_storage(g_svc, db);

    // 3. Compute Plane
    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    service_set_stream_engine(g_svc, engine);

    // 4. Signals
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    LOG_INFO(LOG_CAT_SCHED, "VPP Demo LIVE. Use Ctrl+C to stop.");
    int ret = service_run(g_svc);

    // 5. Cleanup
    stream_engine_destroy(engine);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);
    service_delete(g_svc);
    edge_log_fini();

    return ret;
}
