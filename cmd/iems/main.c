/**
 * @file main.c
 * @brief IEMS Controller Entry Point (Architecturally Refined)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;

static void handle_sigpwr(int sig) {
    (void)sig;
    if (g_svc) {
        LOG_INFO(LOG_CAT_SCHED, "Physical Power Off detected! (SIGPWR/SIGTERM)");
        service_stop(g_svc);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    edge_log_init(NULL, false);
    LOG_INFO(LOG_CAT_SCHED, "Initializing IEMS Controller...");

    uv_loop_t *loop = uv_default_loop();

    // 1. 地基 (Foundation)
    datahub_t hub;
    datahub_init(&hub, loop, "iems_v1", 1024);

    // [修正] 使用正确的时序策略枚举
    sqlite_config_t db_cfg = {.strategy = STRATEGY_SERIES, .db_path = "iems_data.db"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);

    arbiter_t *arb;
    arbiter_init(&arb, loop);

    // 2. 控制面 (Control Plane)
    service_options_t opts = {.mode = "PRODUCTION"};
    g_svc = service_new(&opts);
    service_set_datahub(g_svc, &hub);
    service_set_storage(g_svc, db);
    service_set_arbiter(g_svc, arb);

    // 3. 计算平面 (Compute Plane)
    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    service_set_stream_engine(g_svc, engine);

    // 4. 注册 S14 掉电监听
    signal(SIGPWR, handle_sigpwr);
    signal(SIGTERM, handle_sigpwr);

    // 5. 启动系统
    LOG_INFO(LOG_CAT_SCHED, "IEMS LIVE. Entering Loop...");
    int ret = service_run(g_svc);

    // 6. 物理清理
    stream_engine_destroy(engine);
    arbiter_shutdown(arb);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);
    service_delete(g_svc);
    edge_log_fini();

    return ret;
}