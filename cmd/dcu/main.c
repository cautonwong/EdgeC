#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contract_registry.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "edge_service.h"
#include "io_executor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "sqlite_service.h"

// --- Global Contexts (Exported for Poller Fast-Track) ---
dcu_inventory_t g_dcu_inventory;
dcu_poller_t g_dcu_poller;
dcu_storage_t g_dcu_storage;

static service_t *g_running_svc = NULL;
static uv_timer_t g_freeze_trigger_timer;

typedef struct {
    datahub_t *hub;
} sync_ctx_t;

static sync_ctx_t g_sync_ctx;

// Linker Stubs
int northbound_mgr_start(void *mgr) { (void)mgr; return 0; }
void northbound_mgr_stop(void *mgr) { (void)mgr; }

static void on_sigterm(int sig) {
    (void)sig;
    LOG_INFO(LOG_CAT_SCHED, "DCU: Received termination signal, flushing data...");
    dcu_storage_flush(&g_dcu_storage);
    if (g_running_svc) service_stop(g_running_svc);
}

// 影子映射：物理矩阵映射 (Index = MeterIdx * 10 + Offset)
static int dcu_tag_map_fn(uint32_t tag_id, uint32_t device_id) {
    uint32_t offset = 0;
    if (tag_id == 0x20000200 || tag_id == 1004) offset = 0;
    else return -1;
    return (int)(device_id * 10 + offset);
}

static void _freeze_trigger_cb(uv_timer_t *handle) {
    sync_ctx_t *ctx = (sync_ctx_t *)handle->data;
    pipe_msg_t msg;
    int saved = 0;
    
    for(int i = 0; i < g_dcu_inventory.count; i++) {
        int shadow_idx = dcu_tag_map_fn(0x20000200, (uint32_t)i);
        if (datahub_read_shadow_idx(ctx->hub, (uint32_t)shadow_idx, &msg) == 0) {
            dcu_storage_write_telemetry(&g_dcu_storage, g_dcu_inventory.meters[i].address, (uint32_t)i, 0x20000200, (float)msg.payload.val.f);
            saved++;
        }
    }
    if (saved > 0) {
        dcu_storage_flush(&g_dcu_storage);
        LOG_INFO(LOG_CAT_SCHED, "✅ [STORAGE] SUCCESS! Captured %d meters truth data.", saved);
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    LOG_INFO(LOG_CAT_SCHED, "🚀 Starting EdgeC DCU (Series 6xxx) - Zero-Blocking Performance Mode");

    uv_loop_t *loop = uv_default_loop();
    extern const edge_contract_t g_contract_manifest;
    contract_register(&g_contract_manifest);

    dcu_storage_init(&g_dcu_storage, loop, "dcu_runtime.db");
    
    datahub_t hub;
    datahub_init(&hub, loop, "dcu_production_bus", 32768, 32768, NULL, 0, NULL, NULL, NULL);
    hub.map_fn_v2 = dcu_tag_map_fn; 

    arbiter_t *arbiter_ctx = NULL;
    arbiter_init(&arbiter_ctx, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    
    extern const edge_driver_t dlt698_driver;
    extern const edge_driver_t dlt645_driver;
    driver_manager_register(drv_mgr, &dlt698_driver);
    driver_manager_register(drv_mgr, &dlt645_driver);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter_ctx, drv_mgr, 64, 8192, 4096);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);
    arbiter_resume(arbiter_ctx);

    const driver_contract_injection_t *inj_dlt698 = contract_model_find("DLT698");
    const driver_contract_injection_t *inj_dlt645 = contract_model_find("DLT645");

    // 建立 32 个物理通道 (0..31)
    for (int i = 0; i < 32; i++) {
        char path[64]; snprintf(path, sizeof(path), "/tmp/rs485_%d.sock", i);
        const driver_contract_injection_t *inj = (i < 22) ? inj_dlt698 : inj_dlt645;
        const char *proto = (i < 22) ? "DLT698" : "DLT645";

        if (inj) bus_orchestrator_setup_master(io_orch, i, proto, 0, inj);

        struct sockaddr_un addr; memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX; strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);
        
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        // [Zero-Blocking] 严禁在主线程使用 sleep，只尝试一次，连不上就让它去
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) >= 0) {
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t *h = io_executor_create_from_fd(loop, fd, hub.io_pool);
            bus_orchestrator_attach_io(io_orch, i, h);
        } else {
            close(fd);
        }
    }

    dcu_inventory_init(&g_dcu_inventory);
    dcu_inventory_load_from_db(&g_dcu_inventory, "dcu_runtime.db");

    dcu_poller_init(&g_dcu_poller);
    uint32_t oads[1] = {0x20000200}; 
    dcu_poller_add_scheme(&g_dcu_poller, 1, 0, oads, 1);
    dcu_poller_add_task(&g_dcu_poller, 1, 5, 1); 

    service_t *svc = service_new(loop, &hub, arbiter_ctx, NULL, NULL, NULL, NULL);
    g_running_svc = svc;

    signal(SIGTERM, on_sigterm); signal(SIGINT, on_sigterm);
    dcu_poller_start(&g_dcu_poller, &g_dcu_inventory, io_orch, &hub, drv_mgr, loop, &g_dcu_storage);

    uv_timer_init(loop, &g_freeze_trigger_timer);
    g_freeze_trigger_timer.data = &g_sync_ctx;
    g_sync_ctx.hub = &hub;
    // 5 秒后启动采集
    uv_timer_start(&g_freeze_trigger_timer, _freeze_trigger_cb, 5000, 5000);

    bus_orchestrator_run(io_orch);
    return service_run(svc);
}
