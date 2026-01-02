/**
 * @file main.c
 * @brief EdgeOS DCU 生产级总装 (加固版)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bus_orchestrator.h"
#include "contracts.h"
#include "dcu_inventory.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_service.h"
#include "sched_time.h"

extern int dlt645_driver_register(void);
static service_t *g_svc = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    printf("\n[DCU] SIGINT received, stopping...\n");
    if (g_svc) service_stop(g_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // 强制输出到标准输出，确证启动
    printf("=== EdgeOS DCU Starting Up ===\n");

    edge_log_init(NULL, false);  // 先输出到控制台

    // 0. 契约一致性自检 (工业级防护)
    if (edge_contract_self_test() != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Contract Integrity Failed! Hardware bindings mismatch. Aborting.");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    service_options_t opts = {.mode = "DCU_PROD_V1"};
    g_svc = service_new(&opts);
    uv_loop_t *loop = uv_default_loop();

    // 1. DataHub
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    if (datahub_init(hub, loop, "dcu_shm_v1", 1024) != 0) {
        fprintf(stderr, "[FATAL] Failed to init DataHub\n");
        return 1;
    }
    service_set_datahub(g_svc, hub);

    // 2. Arbiter
    arbiter_t *arb;
    arbiter_init(&arb, loop);
    service_set_arbiter(g_svc, arb);

    // 3. SQLite (档案持久化)
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = "dcu_inventory.db"};
    sqlite_svc_t *db = sqlite_svc_create(loop, &db_cfg);
    service_set_storage(g_svc, db);

    // 4. 驱动与分发
    driver_manager_init();
    dlt645_driver_register();
    edge_driver_t *dlt645 = driver_manager_find("DLT645-2007");

    for (int i = 0; i < 4; i++) {
        bus_orchestrator_t *orch = bus_orchestrator_create(dlt645, hub, arb);
        bus_orchestrator_run(orch);
    }

    // 5. 档案加载 (首次启动时添加档案，后续从 DB 加载)
    dcu_inventory_add_meter(g_svc, "Meter_485_01", 0, 10001, "DLT645");
    dcu_inventory_add_meter(g_svc, "Meter_HPLC_01", 3, 20001, "DLT645");
    dcu_inventory_load_and_run(g_svc);

    printf("[DCU] All planes ready. Entering Main Loop...\n");
    return service_run(g_svc);
}
