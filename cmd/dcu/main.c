#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
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

static service_t *g_running_svc = NULL;
static uv_timer_t g_freeze_trigger_timer;

typedef struct {
    datahub_t *hub;
} sync_ctx_t;

static sync_ctx_t g_sync_ctx;

// --- 【实事求是】严格对齐头文件的 Linker 桩函数 ---
int northbound_mgr_start(northbound_mgr_t *mgr) { (void)mgr; return 0; }
void northbound_mgr_stop(northbound_mgr_t *mgr) { (void)mgr; }

// --- Signal Handling ---
static void on_sigterm(int sig) {
    (void)sig;
    printf("\n⚠️  [WARN] SIGTERM 触发！原子落盘并安全退出...\n");
    dcu_storage_flush(&g_dcu_storage);
    if (g_running_svc) service_stop(g_running_svc);
}

// DataHub 索引计算：这里我们由于采用“物理索引直通”模式，直接返回 tag_id
static int dcu_tag_map_fn(uint32_t tag_id) {
    return (int)tag_id; 
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    sync_ctx_t *ctx = (sync_ctx_t *)handle->data;
    printf("\n📅 [FREEZE] 2000 只表全断面数字孪生数据原子快照入库...\n");
    
    pipe_msg_t msg;
    int saved = 0;
    
    for(int i = 0; i < g_dcu_inventory.count; i++) {
        production_meter_t *m = &g_dcu_inventory.meters[i];
        
        // 【核心闭环】读取该表专属的影子坑位 (Index = i * 10 + 0)
        uint32_t shadow_idx = (uint32_t)i * 10 + 0; 

        if (datahub_read_shadow(ctx->hub, shadow_idx, &msg) == 0 && msg.payload.val.f > 1.0) {
            dcu_storage_write_telemetry(&g_dcu_storage, m->address, 0x20000200, msg.payload.val.f);
            saved++;
        }
    }
    
    dcu_storage_flush(&g_dcu_storage);
    printf("💾 [FREEZE] 存档完毕：已完成 %d 条明细的真实物理数值存盘。\n", saved);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("🚀 Starting Production-Grade DLT 698.45 DCU (Full Shadow Matrix Mode)...\n");

    uv_loop_t *loop = uv_default_loop();
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);
    const edge_contract_t *contract = contract_get_active();

    dcu_storage_init(&g_dcu_storage, loop, "dcu_runtime.db");
    
    // 【关键扩容】初始化 32768 个内存影子槽位，支持 2000 只表的独立状态保持
    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 32768, 32768, NULL, 0, NULL, dcu_tag_map_fn, NULL);

    arbiter_t *arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    extern const edge_driver_t dlt698_driver;
    driver_manager_register(drv_mgr, &dlt698_driver);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter_ctx, drv_mgr, DCU_MAX_CHANNELS, 256, 4096);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    arbiter_resume(arbiter_ctx);

    const driver_contract_injection_t *inj_dlt698 = edge_model_find("DLT698");

    // 动态发现 Manifest 定义的所有通道
    for (uint32_t i = 0; i < contract->connectivity_count; i++) {
        const edge_connectivity_t *c = &contract->connectivity[i];
        if (strcmp(c->category, "SOUTHBOUND") != 0) continue;
        const char *real_path = edge_resolve_bus_path(c->id);
        if (!real_path) continue;

        struct sockaddr_un uds_addr;
        memset(&uds_addr, 0, sizeof(uds_addr));
        uds_addr.sun_family = AF_UNIX;
        strncpy(uds_addr.sun_path, real_path, sizeof(uds_addr.sun_path) - 1);

        int uds_fd = -1;
        for (int r = 0; r < 5; r++) { // 严谨连接，绝不敷弄
            uds_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (connect(uds_fd, (struct sockaddr *)&uds_addr, sizeof(uds_addr)) >= 0) break;
            close(uds_fd); uds_fd = -1;
            usleep(200000);
        }
        if (uds_fd >= 0) {
            io_handle_t *handle = io_executor_create_from_fd(loop, uds_fd, hub.io_pool);
            bus_orchestrator_attach_io(io_orch, c->id, handle);
            printf("  🔗 Channel %d: Physically Bound to %s\n", c->id, real_path);
        }
        // 关键：将修复后的 DLT698 哈希注入每一条通道
        if (inj_dlt698) bus_orchestrator_setup_master(io_orch, c->id, "DLT698", 1, inj_dlt698);
    }

    dcu_inventory_init(&g_dcu_inventory);
    dcu_inventory_load_from_db(&g_dcu_inventory, "dcu_runtime.db");
    
    dcu_poller_init(&g_dcu_poller);
    // 方案配置
    uint32_t oads[1] = {0x20000200}; // A 相电压
    dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 1);
    dcu_poller_add_task(&g_dcu_poller, 1, 60, 1);

    service_t *svc = service_new(loop, &hub, arbiter_ctx, NULL, NULL, NULL, NULL);
    g_running_svc = svc;
// 【7】运行
signal(SIGTERM, on_sigterm); signal(SIGINT, on_sigterm);
dcu_poller_start(&g_dcu_poller, &g_dcu_inventory, io_orch, &hub, drv_mgr, loop);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = &hub;
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    bus_orchestrator_run(io_orch);
    return service_run(svc);
}
