/**
 * @file main.c
 * @brief IEMS Controller - The Ultimate Composition Root (Sprint 3 Reinforced)
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

extern int dlt645_driver_register(driver_manager_t *mgr);
extern int dlms_driver_register(driver_manager_t *mgr);
extern int iec104_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // 1. 基础
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    // 2. 控制面与驱动管理
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    dlms_driver_register(drv_mgr);
    dlt645_driver_register(drv_mgr);
    iec104_driver_register(drv_mgr);

    // 3. 数据面 (初始化不带 Pool)
    datahub_t hub;
    datahub_init(&hub, loop, "iems_v4.5", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    // 4. 执行面 (创建 Orchestrator 并注入池到 DataHub)
    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);  // 延迟物理注入

    // 5. 存储与业务
    sqlite_config_t db_conf = {
        .db_path = "iems.db", .strategy = STRATEGY_LITE, .max_db_size_mb = 100};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);
    edge_params_t *param_mgr = edge_params_create(db_service);

    // 6. 北向
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "iems", db_service);
    north_mgr->user_data = NULL;  // 稍后在 service_new 中关联

    // 7. 总装
    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;
    north_mgr->user_data = svc;  // 闭环关联

    // 8. 启动
    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    // 9. 销毁
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
    edge_log_destroy(logger);

    return exit_code;
}
