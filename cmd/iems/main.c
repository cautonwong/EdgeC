/**
 * @file main.c
 * @brief IEMS Controller - The Ultimate Composition Root
 *
 * 架构宣言：
 * 1. src 目录下零全局变量。
 * 2. 所有组件在 main 中显式 create。
 * 3. 依赖关系通过构造函数 (xxx_new/create) 显式注入。
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

// 驱动注册接口 (现在需要 mgr 指针)
extern int dlt645_driver_register(driver_manager_t *mgr);
extern int dlms_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // =================================================================================
    // STEP 1: Component Creation (创建原子零件)
    // =================================================================================

    // 1.1 基础设施 (Infrastructure)
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);  // 显式创建日志对象

    // 1.2 数据面 (Data Plane)
    datahub_t hub;
    datahub_init(&hub, loop, "iems_v4.5", 1024);

    // 1.3 存储面 (Storage Plane - 可随时替换实现)
    sqlite_config_t db_conf = {.db_path = "iems_main.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);

    // 1.4 控制面 (Control Plane)
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);

    // 1.5 驱动工厂 (Driver Manager - 零全局状态)
    driver_manager_t *drv_mgr = driver_manager_create();
    dlms_driver_register(drv_mgr);
    dlt645_driver_register(drv_mgr);

    // 1.6 业务引擎 (Logic Engines)
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);
    edge_params_t *param_mgr = edge_params_create(db_service);
    edge_params_register_metadata(param_mgr, g_param_metadata, g_param_metadata_len);

    // 1.7 编排器 (IO Orchestrator)
    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 16, 2048);

    // 1.8 北向服务 (Northbound Stack)
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring);
    north_mgr->rest_port = 8081;
    north_mgr->web_port = 8080;

    // =================================================================================
    // STEP 2: Assembly & Injection (总装与注入)
    // =================================================================================

    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;

    // =================================================================================
    // STEP 3: Execution (启动)
    // =================================================================================

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);

    edge_log_out(logger, LOG_CAT_CORE, 0,
                 "EdgeOS IEMS v4.5 Composition Root initialized successfully.");
    int exit_code = service_run(svc);

    // =================================================================================
    // STEP 4: Teardown (反向销毁)
    // =================================================================================

    service_delete(svc);
    northbound_mgr_stop(north_mgr);
    free(north_mgr);
    bus_orchestrator_destroy(io_orch);
    stream_engine_destroy(stream_engine);
    driver_manager_destroy(drv_mgr);
    arbiter_destroy(arbiter);
    edge_params_destroy(param_mgr);
    sqlite_svc_destroy(db_service);
    datahub_destroy(&hub);
    edge_log_destroy(logger);  // 最后销毁日志

    return exit_code;
}