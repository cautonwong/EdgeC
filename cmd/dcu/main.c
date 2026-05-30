#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/un.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "contract_registry.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_params.h"
#include "edge_service.h"
#include "io_executor.h"
#include "northbound_mgr.h"
#include "stream_engine.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "dcu_router.h"
#include "dcu_security.h"
#include "dcu_discovery.h"
#include "dcu_event_monitor.h"

// --- Global Contexts ---
static dcu_inventory_t g_dcu_inventory;
static dcu_poller_t g_dcu_poller;
static dcu_storage_t g_dcu_storage;
static dcu_esam_t g_dcu_esam;
static dcu_discovery_t g_dcu_discovery;
static dcu_event_obj_t g_dcu_evt_3000;
static dcu_event_obj_t g_dcu_evt_3005;

static service_t *g_running_svc = NULL;
static uv_timer_t g_sync_timer;
static uv_timer_t g_freeze_trigger_timer;

typedef struct {
    datahub_t *hub;
    stream_engine_t *engine;
} sync_ctx_t;

static sync_ctx_t g_sync_ctx;

// --- Signal Handling ---
static void on_sigterm(int sig) {
    (void)sig;
    printf("\n⚠️  [WARN] SIGTERM/停电中断触发！启动紧急掉电原子保护，强制执行缓存落盘...\n");
    dcu_storage_flush(&g_dcu_storage);
    dcu_poller_stop(&g_dcu_poller);
    if (g_running_svc) service_stop(g_running_svc);
}

// --- Callbacks ---
static void _sync_datahub_to_stream_cb(uv_timer_t *handle) {
    sync_ctx_t *ctx = (sync_ctx_t *)handle->data;
    pipe_msg_t msg;
    if (datahub_read_shadow(ctx->hub, OAD_VOLTAGE, &msg) == 0) {
        stream_engine_push(ctx->engine, &msg);
    }
    if (datahub_read_shadow(ctx->hub, OAD_CURRENT, &msg) == 0) {
        stream_engine_push(ctx->engine, &msg);
    }
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    sync_ctx_t *ctx = (sync_ctx_t *)handle->data;
    pipe_msg_t freeze_msg;
    pipe_msg_init_scalar(&freeze_msg, OAD_DAILY_FREEZE, 1.0, (uint64_t)time(NULL));
    stream_engine_push(ctx->engine, &freeze_msg);
}

static void _on_remote_control_timer(uv_timer_t *handle) {
    printf("\n📡 [REMOTE] 主站下发远程指令：OAD 8000 01 00 (拉闸)\n");
    dcu_router_action(0x80000100, NULL, NULL);
    uv_timer_stop(handle);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("🚀 Starting Production-Grade DLT 698.45 DCU Service Engine...\n");

    uv_loop_t *loop = uv_default_loop();

    // 【1】首先加载契约字典，它是所有路由映射的基础
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);

    // 【2】基础服务与驱动初始化
    int ret = dcu_storage_init(&g_dcu_storage, loop, "dcu_runtime.db");
    if (ret != 0) {
        printf("[FATAL] SQLite storage initialization failed!\n");
        return -1;
    }

    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 1024, g_tag_metadata_len, g_tag_metadata, g_tag_metadata_len,
                 NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    arbiter_t *arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    
    extern const edge_driver_t modbus_rtu_driver;
    extern const edge_driver_t dlt645_driver;
    extern const edge_driver_t dlt698_driver;
    driver_manager_register(drv_mgr, &modbus_rtu_driver);
    driver_manager_register(drv_mgr, &dlt645_driver);
    driver_manager_register(drv_mgr, &dlt698_driver);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter_ctx, drv_mgr, 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    arbiter_resume(arbiter_ctx);

    // 【3】物理信道绑定与契约注入
    const char *uds_path = "/tmp/rs485_1.sock";
    int uds_fd = -1;
    struct sockaddr_un uds_addr;
    if ((uds_fd = socket(AF_UNIX, SOCK_STREAM, 0)) >= 0) {
        memset(&uds_addr, 0, sizeof(uds_addr));
        uds_addr.sun_family = AF_UNIX;
        strncpy(uds_addr.sun_path, uds_path, sizeof(uds_addr.sun_path) - 1);
        if (connect(uds_fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr)) >= 0) {
            printf("🔗 [MATRIX] Connected to physical simulator at %s\n", uds_path);
        } else {
            close(uds_fd); uds_fd = 1; 
        }
    }
    io_handle_t *rs485_1 = io_executor_create_from_fd(loop, uds_fd, hub.io_pool);
    bus_orchestrator_attach_io(io_orch, 1, rs485_1);

    // 核心：基于规约名称获取模型注入包
    const driver_contract_injection_t *inj_modbus = edge_model_find("ModbusRTU");
    const driver_contract_injection_t *inj_dlt645 = edge_model_find("DLT645");
    const driver_contract_injection_t *inj_dlt698 = edge_model_find("DLT698");
    if (!inj_dlt698) inj_dlt698 = inj_dlt645;

    bus_orchestrator_setup_master(io_orch, 1, "ModbusRTU", 1, inj_modbus);
    bus_orchestrator_setup_master(io_orch, 1, "DLT645", 1, inj_dlt645);
    bus_orchestrator_setup_master(io_orch, 1, "DLT698", 1, inj_dlt698);

    // 【4】业务模块初始化
    dcu_inventory_init(&g_dcu_inventory);
    dcu_inventory_load_from_db(&g_dcu_inventory, "dcu_runtime.db");
    if (g_dcu_inventory.count == 0) {
        dcu_inventory_add(&g_dcu_inventory, "10000001", "DLT645", 9600, 1, inj_dlt645);
        dcu_inventory_add(&g_dcu_inventory, "10000002", "ModbusRTU", 9600, 1, inj_modbus);
        dcu_inventory_add(&g_dcu_inventory, "10000003", "DLT698", 9600, 1, inj_dlt698);
    }
// 5. 初始化业务模块 (任务与方案)
dcu_poller_init(&g_dcu_poller);

// 方案1: 6015 普通采集方案 (抄读电压电流)
uint32_t oads[2] = {OAD_VOLTAGE, OAD_CURRENT};
dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 2);
dcu_poller_add_task(&g_dcu_poller, 1, 5, 1);

// 方案2: 6017 事件采集方案 (演示多态分流)
g_dcu_poller.schemes[g_dcu_poller.scheme_count].scheme_id = 2;
g_dcu_poller.schemes[g_dcu_poller.scheme_count].type = SCHEME_TYPE_EVENT;
g_dcu_poller.scheme_count++;
dcu_poller_add_task(&g_dcu_poller, 2, 7, 2);

// 方案3: 7011 脚本方案 (演示多态分流)
g_dcu_poller.schemes[g_dcu_poller.scheme_count].scheme_id = 3;
g_dcu_poller.schemes[g_dcu_poller.scheme_count].type = SCHEME_TYPE_SCRIPT;
g_dcu_poller.scheme_count++;
dcu_poller_add_task(&g_dcu_poller, 3, 9, 3);

    dcu_security_init(&g_dcu_esam);
    dcu_discovery_init(&g_dcu_discovery);
    g_dcu_evt_3000.oi = 0x3000; g_dcu_evt_3000.threshold = 176.0f; g_dcu_evt_3000.is_enabled = true;
    g_dcu_evt_3005.oi = 0x3005; g_dcu_evt_3005.threshold = 5.0f; g_dcu_evt_3005.is_enabled = true;

    dcu_router_init_ex(&g_dcu_inventory, &g_dcu_poller, &g_dcu_storage, 
                      &g_dcu_esam, &g_dcu_discovery, &g_dcu_evt_3000, &g_dcu_evt_3005);

    // 【5】微内核服务编排
    sqlite_config_t db_conf = {.db_path = "dcu_runtime.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL, NULL, edge_stage_factory_get_registry());
    edge_params_t *param_mgr = edge_params_create(db_service, &hub);
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "dcu", db_service);

    service_t *svc = service_new(loop, &hub, arbiter_ctx, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;

    pipeline_t *evt_pipeline = stream_engine_add_pipeline(stream_engine, "EVENT_PIPELINE", 0);
    pipeline_stage_t *mon_stage = stage_event_monitor_create(&g_dcu_evt_3000, &g_dcu_evt_3005);
    pipeline_add_stage(evt_pipeline, mon_stage->name, mon_stage->process, mon_stage->priv_state, mon_stage->cleanup_state, NULL, NULL);
    free(mon_stage);

    pipeline_t *freeze_pipeline = stream_engine_add_pipeline(stream_engine, "FREEZE_PIPELINE", 0);
    pipeline_stage_t *freeze_stage = stage_periodic_freeze_create_ex(&g_dcu_storage, &hub);
    pipeline_add_stage(freeze_pipeline, freeze_stage->name, freeze_stage->process, freeze_stage->priv_state, freeze_stage->cleanup_state, NULL, NULL);
    free(freeze_stage);

    // 【6】启动运行
    signal(SIGTERM, on_sigterm); signal(SIGINT, on_sigterm);
    dcu_poller_start(&g_dcu_poller, &g_dcu_inventory, io_orch, drv_mgr, loop);

    uv_timer_init(loop, &g_sync_timer);
    g_sync_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = &hub; g_sync_ctx.engine = stream_engine;
    uv_timer_start(&g_sync_timer, _sync_datahub_to_stream_cb, 1000, 1000);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    uv_timer_t control_timer;
    uv_timer_init(loop, &control_timer);
    uv_timer_start(&control_timer, _on_remote_control_timer, 10000, 0);

    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    dcu_poller_stop(&g_dcu_poller);
    dcu_storage_destroy(&g_dcu_storage);
    return exit_code;
}
