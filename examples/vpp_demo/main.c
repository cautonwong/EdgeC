#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "north_driver_adapter.h"
#include "north_source_datahub_adapter.h"
#include "snf_sqlite_adapter.h"
#include "edge_params_sqlite_adapter.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "stream_bus_datahub_adapter.h"

extern int modbus_driver_register(driver_manager_t* mgr);

static service_t* g_running_svc = NULL;
static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    uv_loop_t* loop = uv_default_loop();
    edge_log_t* logger = edge_log_create(NULL, true);

    arbiter_t* arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_storage_t drv_mgr_storage = {0};
    driver_manager_t* drv_mgr = driver_manager_create_in(&drv_mgr_storage);
    modbus_driver_register(drv_mgr);

    datahub_t hub;
    datahub_init(&hub, loop, "vpp_demo", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, NULL, edge_tag_get_compact_index, edge_router_needs_wakeup);

    bus_orchestrator_t* io_orch = bus_orchestrator_create_hub_default(&hub, arbiter, driver_manager_session_port_ptr(drv_mgr), 8, 32, 2048);
    hub.io_pool = bus_orchestrator_get_pool(io_orch);

    sqlite_config_t db_conf = {.db_path = "vpp.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t* db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t* stream_engine =
        stream_engine_create_datahub(&hub, uv_default_loop(), NULL, edge_stage_factory_get_registry());
    edge_params_store_t param_store = edge_params_sqlite_store(db_service);
    edge_bus_datahub_port_t param_port = edge_bus_datahub_port(&hub);
    edge_params_storage_t param_storage;
    edge_params_t* param_mgr = edge_params_create_in(&param_storage, &param_store, &param_port);

    northbound_mgr_t* north_mgr = calloc(1, sizeof(northbound_mgr_t));
    snf_store_port_t snf_store = snf_sqlite_store_port(db_service);
    nb_source_ops_t nb_source = northbound_source_from_datahub(&hub);
    nb_transport_ops_t transport = {0};
    northbound_mgr_init_ex(north_mgr, nb_source, "vpp_demo", &snf_store, transport);
    static north_driver_adapter_storage_t north_driver_storage;
    northbound_mgr_set_driver_port(north_mgr,
                                   north_driver_adapter_init(&north_driver_storage));

    service_t* svc =
        service_new(loop, &hub, arbiter, stream_engine, param_mgr);
    g_running_svc = svc;

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);
    northbound_mgr_start(north_mgr);
    int exit_code = service_run(svc);

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