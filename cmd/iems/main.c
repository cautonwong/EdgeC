/**
 * @file main.c
 * @brief IEMS Controller Entry Point (Architecturally Refined)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "policy_syncer.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;
static northbound_mgr_t g_north;

static void l2_thread_entry(void *arg) {
    datahub_t *hub = (datahub_t *)arg;
    uv_loop_t l2_loop;
    uv_loop_init(&l2_loop);

    LOG_INFO(LOG_CAT_SCHED, "L2 Stream Thread starting...");
    stream_engine_t *engine = stream_engine_create_ex(hub, &l2_loop, NULL);

    uv_run(&l2_loop, UV_RUN_DEFAULT);

    stream_engine_destroy(engine);
    uv_loop_close(&l2_loop);
    LOG_INFO(LOG_CAT_SCHED, "L2 Stream Thread exiting.");
}

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

    // 0. 契约一致性自检 (启动守门员)
    if (edge_contract_self_test() != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Contract Self-test FAILED! Aborting.");
        return EXIT_FAILURE;
    }

    LOG_INFO(LOG_CAT_SCHED, "Initializing IEMS Controller...");

    uv_loop_t *loop = uv_default_loop();

    // 1. 地基 (Foundation)
    datahub_t hub;
    datahub_init(&hub, loop, "iems_v1", 1024);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_SERIES, .db_path = "iems_data.db"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);

    arbiter_t *arb;
    arbiter_init(&arb, loop);

    // 2. 执行面 (Execution Plane)
    extern edge_driver_t dlt645_driver;
    bus_orchestrator_t *orch = bus_orchestrator_create(&dlt645_driver, &hub, arb);

    // 3. 策略同步 (Scenario S1: SQLite Driven)
    policy_syncer_run(db, orch);

    // 4. 控制面 (Control Plane)
    service_options_t opts = {.mode = "PRODUCTION"};
    g_svc = service_new(&opts);
    service_set_datahub(g_svc, &hub);
    service_set_storage(g_svc, db);
    service_set_arbiter(g_svc, arb);

    // 5. 启动北向服务栈 (Web UI & REST API)
    northbound_mgr_init(&g_north, loop, &hub.ring);
    g_north.web_port = 8080;
    g_north.rest_port = 8081;
    g_north.user_data = g_svc;

    // 自适应 Web 根目录探测 (针对开发与部署场景)
    const char *candidates[] = {
        "../www",                     // 场景 A: 在 build/products/iems/bin 运行
        "./build/products/iems/www",  // 场景 B: 在项目根目录运行 (您的场景)
        "./www"                       // 场景 C: 直接在产品目录运行
    };

    bool found = false;
    for (int i = 0; i < 3; i++) {
        char test_path[512];
        snprintf(test_path, sizeof(test_path), "%s/index.html", candidates[i]);
        if (access(test_path, F_OK) == 0) {
            strncpy(g_north.web_root, candidates[i], sizeof(g_north.web_root) - 1);
            LOG_INFO(LOG_CAT_CORE, "Web Assets found at: %s", g_north.web_root);
            found = true;
            break;
        }
    }

    if (!found) {
        LOG_WARN(LOG_CAT_CORE, "Web Assets NOT FOUND. UI will be unavailable.");
        strncpy(g_north.web_root, "./www", sizeof(g_north.web_root) - 1);
    }

    northbound_mgr_start(&g_north);

    // 6. 启动 L2 计算平面线程 (Compute Plane Isolation)
    uv_thread_t l2_thread;
    uv_thread_create(&l2_thread, l2_thread_entry, &hub);

    // 7. 注册 S14 掉电监听
    signal(SIGPWR, handle_sigpwr);
    signal(SIGTERM, handle_sigpwr);

    // 8. 启动系统 (L1 Loop)
    LOG_INFO(LOG_CAT_SCHED, "IEMS LIVE. Entering Loop...");
    bus_orchestrator_run(orch);
    int ret = service_run(g_svc);

    // 9. 物理清理
    northbound_mgr_stop(&g_north);
    service_stop(g_svc);
    uv_thread_join(&l2_thread);

    bus_orchestrator_destroy(orch);
    arbiter_shutdown(arb);
    sqlite_svc_destroy(db);
    datahub_destroy(&hub);
    service_delete(g_svc);
    edge_log_fini();

    return ret;
}