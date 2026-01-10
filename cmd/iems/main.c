/**
 * @file main.c
 * @brief IEMS Controller - The Ultimate Composition Root
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

// 驱动注册接口
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

    // 1.1 基础设施
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    // 1.2 数据面 (注入元数据与唤醒策略)
    datahub_t hub;
    // edge_router_needs_wakeup 由 edge_router_table.c 提供，总是存在
    datahub_init(&hub, loop, "iems_v4.5", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, edge_tag_get_compact_index, edge_router_needs_wakeup);

    // 1.3 驱动与控制面
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);

    driver_manager_t *drv_mgr = driver_manager_create();
    dlms_driver_register(drv_mgr);
    dlt645_driver_register(drv_mgr);
    iec104_driver_register(drv_mgr);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);

    // ---------------------------------------------------------------------------------
    // 注入演示：配置总线 0 为 IEC104 从站，并根据特征门控注入契约 Bundle
    // ---------------------------------------------------------------------------------
    driver_contract_injection_t inject = {0};
#ifdef CONFIG_IEC104_ENABLED
    inject.lookup_by_key = (const void *(*)(uint32_t))edge_get_map_iec104;
    inject.lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_iec104_by_tag;
    inject.meta_list = (const void *const *)g_iec104_gi_list;
    inject.meta_list_len = g_iec104_gi_len;
#endif
    bus_orchestrator_setup_slave(io_orch, 0, "IEC104", 1, &inject);

    // DLMS Master 注入 (示例)
    driver_contract_injection_t dlms_inject = {0};
#ifdef CONFIG_DLMS_ENABLED
    dlms_inject.lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_dlms_by_tag;
#endif
    bus_orchestrator_setup_master(io_orch, 1, "DLMS", 3, &dlms_inject);

    // Modbus 注入
    driver_contract_injection_t mb_inject = {0};
#ifdef CONFIG_MODBUS_ENABLED
    mb_inject.lookup_by_key = (const void *(*)(uint32_t))edge_get_map_modbus;
#endif
    bus_orchestrator_setup_master(io_orch, 2, "ModbusRTU", 1, &mb_inject);

    // 1.4 其他服务
    sqlite_config_t db_conf = {
        .db_path = "iems.db", .strategy = STRATEGY_LITE, .cache_size_kb = 2048};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);
    edge_params_t *param_mgr = edge_params_create(db_service);
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring);

    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    // 清理
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