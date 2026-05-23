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

/* --- 引入生产级 DLT 698.45 核心组件 (Task 6) --- */
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"

// 全局运行时句柄，便于停电保护及 SIGTERM 销毁
static service_t *g_running_svc = NULL;
static dcu_storage_t g_dcu_storage;
static dcu_poller_t g_dcu_poller;
static dcu_inventory_t g_dcu_inventory;

/**
 * @brief 【掉电保护核心回调】接收到 SIGTERM 物理停电中断信号时，强制将内存缓冲原子刷写存盘
 */
static void on_sigterm(int sig) {
    (void)sig;
    printf("\n⚠️  [WARN] SIGTERM/停电中断触发！启动紧急掉电原子保护，强制执行缓存落盘...\n");
    
    // 强行刷盘，通过批量 TRANSACTION 原子存盘，确保计量数据和最后遥测断面 100% 不损坏
    dcu_storage_flush(&g_dcu_storage);
    
    // 停止轮询生产器
    dcu_poller_stop_simulation(&g_dcu_poller);
    
    if (g_running_svc) {
        service_stop(g_running_svc);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("🚀 Starting Production-Grade DLT 698.45 DCU Service Engine...\n");
    
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    // 1. 初始化 SQLite 存储服务与双缓冲批量事务模块
    int ret = dcu_storage_init(&g_dcu_storage, loop, "dcu_production.db");
    if (ret != 0) {
        printf("[FATAL] SQLite storage initialization failed!\n");
        return -1;
    }

    // 2. 初始化电能表注册档案
    dcu_inventory_init(&g_dcu_inventory);
    
    // 自动载入首批工业子表注册信息 (TSA: 10000001 和 10000002)
    dcu_inventory_add(&g_dcu_inventory, "10000001", "dlt698", 9600, 1);
    dcu_inventory_add(&g_dcu_inventory, "10000002", "dlt698", 2400, 1);

    // 3. 初始化高频轮询调度引擎
    dcu_poller_init(&g_dcu_poller);
    
    // 声明并注册方案 1：5秒高频周期采样方案，抄读电压 (OAD 20000200) 与电流 (OAD 20010200)
    uint32_t oads[2] = {0x20000200, 0x20010200};
    dcu_poller_add_scheme(&g_dcu_poller, 1, 5, oads, 2);

    // 4. 初始化系统底层 DataHub 总线底座
    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 1024, g_tag_metadata_len, g_tag_metadata, g_tag_metadata_len,
                 NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    // 5. 挂接南向总线编排器与驱动管理器
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    
    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);

    // 6. 挂接北向前置通信与参数配置管理层
    sqlite_config_t db_conf = {.db_path = "dcu_runtime.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine =
        stream_engine_create(&hub, NULL, NULL, edge_stage_factory_get_registry());
    edge_params_t *param_mgr = edge_params_create(db_service, &hub);
    edge_params_register_metadata(param_mgr, g_param_metadata, g_param_metadata_len);

    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "dcu", db_service);

    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;
    north_mgr->user_data = svc;

    // 7. 【安全第一】注册 SIGTERM 掉电告警中断信号
    signal(SIGTERM, on_sigterm);
    signal(SIGINT, on_sigterm); // 支持 Ctrl+C 干净退出测试

    // 8. 启动物理数采高频发生器 (Task 4 直推 DataHub 影子总线)
    dcu_poller_start_simulation(&g_dcu_poller, &g_dcu_inventory, &hub, loop);
    printf("📊 [DCU] Simulated sensor production loop started (OAD 20000200 / 20010200, 5s interval)\n");

    // 9. 开启总线并发编排并进入 libuv 主事件循环
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    // 10. 退出资源干净清理与回收
    printf("🧹 [DCU] Main loop exited, releasing all allocated resources...\n");
    dcu_poller_stop_simulation(&g_dcu_poller);
    dcu_poller_destroy(&g_dcu_poller);
    dcu_inventory_destroy(&g_dcu_inventory);
    dcu_storage_destroy(&g_dcu_storage);

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
    
    printf("👋 [DCU] DCU engine shutdown gracefully.\n");
    return exit_code;
}