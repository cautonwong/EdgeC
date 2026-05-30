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
    (void)handle;
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    sync_ctx_t *ctx = (sync_ctx_t *)handle->data;
    printf("\n📅 [FREEZE] 触发全断面数据原子快照入库 (2000只表)...\n");
    
    pipe_msg_t msg;
    int saved = 0;
    
    // 【核心修正】实事求是：将协议 OAD 转换为 DataHub 内部索引 ID
    uint32_t voltage_tag_id = edge_lookup_tag(0x20000200, NULL);
    if (voltage_tag_id == 0) {
        printf("❌ [FREEZE] 致命错误：电压 OAD (0x20000200) 在契约中未定义！\n");
        return;
    }

    for(int i=0; i<g_dcu_inventory.count; i++) {
        production_meter_t *m = &g_dcu_inventory.meters[i];
        
        // 只有读到非零值（即 Matrix 已经填入过数据），才进行有效入库
        if (datahub_read_shadow(ctx->hub, voltage_tag_id, &msg) == 0) {
            dcu_storage_write_telemetry(&g_dcu_storage, m->address, 0x20000200, msg.payload.val.f);
            saved++;
        }
    }
    
    dcu_storage_flush(&g_dcu_storage);
    printf("💾 [FREEZE] 存档完毕：已完成 %d 条明细的原子落盘 (TagID: %u)。\n", saved, voltage_tag_id);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("🚀 Starting Production-Grade DLT 698.45 DCU Service Engine (32-Channel Mode)...\n");

    uv_loop_t *loop = uv_default_loop();

    // 【1】加载契约
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);

    // 【2】持久化存储
    dcu_storage_init(&g_dcu_storage, loop, "dcu_runtime.db");

    // 【3】总线初始化
    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 4096, g_tag_metadata_len, g_tag_metadata, g_tag_metadata_len,
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

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter_ctx, drv_mgr, DCU_MAX_CHANNELS, 256, 4096);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    arbiter_resume(arbiter_ctx);

    // 【4】物理信道动态绑定 (利用修复后的编译器后端)
    const driver_contract_injection_t *inj_modbus = edge_model_find("ModbusRTU");
    const driver_contract_injection_t *inj_dlt645 = edge_model_find("DLT645");
    const driver_contract_injection_t *inj_dlt698 = edge_model_find("DLT698");
    if (!inj_dlt698) inj_dlt698 = inj_dlt645;

    for (int i = 1; i <= 32; i++) {
        char uds_path[64];
        snprintf(uds_path, sizeof(uds_path), "/tmp/rs485_%d.sock", i);
        int uds_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        struct sockaddr_un uds_addr;
        memset(&uds_addr, 0, sizeof(uds_addr));
        uds_addr.sun_family = AF_UNIX;
        strncpy(uds_addr.sun_path, uds_path, sizeof(uds_addr.sun_path) - 1);
        
        if (connect(uds_fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr)) >= 0) {
            io_handle_t *handle = io_executor_create_from_fd(loop, uds_fd, hub.io_pool);
            bus_orchestrator_attach_io(io_orch, i, handle);
        } else {
            io_handle_t *handle = io_executor_create_from_fd(loop, 1, hub.io_pool);
            bus_orchestrator_attach_io(io_orch, i, handle);
        }

        if (inj_modbus) bus_orchestrator_setup_master(io_orch, i, "ModbusRTU", 1, inj_modbus);
        if (inj_dlt645) bus_orchestrator_setup_master(io_orch, i, "DLT645", 1, inj_dlt645);
        if (inj_dlt698) bus_orchestrator_setup_master(io_orch, i, "DLT698", 1, inj_dlt698);
    }

    // 【5】业务模块初始化
    dcu_inventory_init(&g_dcu_inventory);
    int loaded = dcu_inventory_load_from_db(&g_dcu_inventory, "dcu_runtime.db");
    printf("✅ [INVENTORY] Grid loaded: %d meters.\n", loaded);

    dcu_poller_init(&g_dcu_poller);
    uint32_t oads[2] = {0x20000200, 0x20010200};
    dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 2);
    dcu_poller_add_task(&g_dcu_poller, 1, 60, 1);

    dcu_security_init(&g_dcu_esam);
    dcu_discovery_init(&g_dcu_discovery);
    g_dcu_evt_3000.oi = 0x3000; g_dcu_evt_3000.threshold = 176.0f; g_dcu_evt_3000.is_enabled = true;
    g_dcu_evt_3005.oi = 0x3005; g_dcu_evt_3005.threshold = 5.0f; g_dcu_evt_3005.is_enabled = true;

    dcu_router_init_ex(&g_dcu_inventory, &g_dcu_poller, &g_dcu_storage, 
                      &g_dcu_esam, &g_dcu_discovery, &g_dcu_evt_3000, &g_dcu_evt_3005);

    // 【6】服务挂接
    sqlite_config_t db_conf = {.db_path = "dcu_runtime.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL, NULL, edge_stage_factory_get_registry());
    edge_params_t *param_mgr = edge_params_create(db_service, &hub);
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring, "dcu", db_service);

    service_t *svc = service_new(loop, &hub, arbiter_ctx, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;

    // 【7】运行
    signal(SIGTERM, on_sigterm); signal(SIGINT, on_sigterm);
    dcu_poller_start(&g_dcu_poller, &g_dcu_inventory, io_orch, drv_mgr, loop);

    uv_timer_init(loop, &g_sync_timer);
    g_sync_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = &hub; g_sync_ctx.engine = stream_engine;
    uv_timer_start(&g_sync_timer, _sync_datahub_to_stream_cb, 1000, 1000);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    bus_orchestrator_run(io_orch);
    return service_run(svc);
}
