/**
 * @file main.c
 * @brief EdgeOS DCU Entry Point (V3 Smoke Test Standard)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "dcu_inventory.h"
#include "driver_manager.h"

static service_t *g_svc = NULL;

extern int io_channel_open(const char *path, int baud);
extern void bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd);

static void handle_sigint(int sig) {
    (void)sig;
    if (g_svc) service_stop(g_svc);
}

static void on_smoke_timeout(uv_timer_t *handle) {
    LOG_INFO(LOG_CAT_CORE, "Smoke Test Watchdog: Terminating DCU.");
    service_stop((service_t *)handle->data);
}

int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));
    edge_log_init(NULL, false);

    const char *io_port = NULL;
    bool smoke_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) io_port = argv[++i];
        else if (strcmp(argv[i], "--smoke-test") == 0) smoke_mode = true;
    }

    if (edge_contract_self_test() != 0) return EXIT_FAILURE;

    printf("=== EdgeOS DCU Starting [V3 Architecture] ===\n");
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    service_options_t opts = {
        .product_name = "dcu_v1",
        .db_path = "dcu_inventory.db",
        .rest_port = 18081,
        .mode = "PRODUCTION"
    };
    g_svc = service_new(&opts);
    if (smoke_mode) service_enable_smoke_test(g_svc);
    
    uv_loop_t *loop = service_get_loop(g_svc);
    datahub_t *hub = service_get_datahub(g_svc);
    arbiter_t *arb = service_get_arbiter(g_svc);

    // 驱动与执行器
    extern int dlt645_driver_register(void);
    driver_manager_init();
    dlt645_driver_register();
    
    bus_orchestrator_t *orch = bus_orchestrator_create(hub, arb, 8, 16, 2048);

    if (io_port) {
        int fd = io_channel_open(io_port, 9600);
        if (fd >= 0) {
            bus_orchestrator_attach_fd(orch, 0, fd);
            // 冒烟测试：统一使用 DeviceID 1
            bus_orchestrator_add_poll(orch, "DLT645-2007", 1, 1001, 1000);
        }
    }

    // 业务逻辑：资产档案 (冒烟测试下跳过复杂加载)
    if (!smoke_mode) {
        dcu_inventory_add_meter(g_svc, "Meter_01", 0, 1, "DLT645");
        dcu_inventory_load_and_run(g_svc);
    }

    uv_timer_t smoke_timer;
    if (smoke_mode) {
        uv_timer_init(loop, &smoke_timer);
        smoke_timer.data = g_svc;
        uv_timer_start(&smoke_timer, on_smoke_timeout, 15000, 0);
    }

    bus_orchestrator_run(orch);
    int ret = service_run(g_svc);

    bus_orchestrator_destroy(orch);
    service_delete(g_svc);
    edge_log_fini();
    return ret;
}
