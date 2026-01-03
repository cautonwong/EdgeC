/**
 * @file main.c
 * @brief EdgeOS DCU 生产级总装 (Container Orchestration Mode)
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

extern int dlt645_driver_register(void);
static service_t *g_svc = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    if (g_svc) service_stop(g_svc);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_init(NULL, false);

    // 0. 契约自检
    if (edge_contract_self_test() != 0) return EXIT_FAILURE;

    printf("=== EdgeOS DCU Starting [v4.5 Container] ===\n");
    signal(SIGINT, handle_sigint);

    // 1. 利用容器自组装
    service_options_t opts = {.product_name = "dcu_shm_v1",
                              .db_path = "dcu_inventory.db",
                              .shm_slots = 1024,
                              .web_port = 18080,
                              .rest_port = 18081,
                              .mode = "PRODUCTION"};
    g_svc = service_new(&opts);

    datahub_t *hub = service_get_datahub(g_svc);
    arbiter_t *arb = service_get_arbiter(g_svc);

    // 2. 驱动与编排
    driver_manager_init();
    dlt645_driver_register();
    edge_driver_t *dlt645 = driver_manager_find("DLT645-2007");

    for (int i = 0; i < 4; i++) {
        bus_orchestrator_t *orch = bus_orchestrator_create(dlt645, hub, arb);
        bus_orchestrator_run(orch);
    }

    // 3. 业务逻辑 (档案)
    dcu_inventory_add_meter(g_svc, "Meter_01", 0, 10001, "DLT645");
    dcu_inventory_load_and_run(g_svc);

    return service_run(g_svc);
}