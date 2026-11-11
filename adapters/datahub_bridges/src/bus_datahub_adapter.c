#include "bus_datahub_adapter.h"

static uv_loop_t *_get_loop(void *ctx) { return datahub_get_loop((datahub_t *)ctx); }

static int _publish(void *ctx, const PipeMsg *msg) {
    return datahub_publish((datahub_t *)ctx, msg);
}

edge_bus_datahub_port_t edge_bus_datahub_port(datahub_t *hub) {
    edge_bus_datahub_port_t port = {
        .ctx = hub,
        .get_loop = _get_loop,
        .publish = _publish,
    };
    return port;
}

bus_orchestrator_t *bus_orchestrator_create_hub(bus_orchestrator_storage_t *storage, datahub_t *hub,
                                                arbiter_t *arb,
                                                const edge_driver_session_port_t *driver_port,
                                                int max_buses, uint32_t pool_size, uint32_t buf_size) {
    edge_bus_datahub_port_t port = edge_bus_datahub_port(hub);
    return bus_orchestrator_create_in(storage, &port, arb, driver_port, max_buses, pool_size, buf_size);
}

bus_orchestrator_t *bus_orchestrator_create_hub_default(datahub_t *hub, arbiter_t *arb,
                                                        const edge_driver_session_port_t *driver_port,
                                                        int max_buses, uint32_t pool_size,
                                                        uint32_t buf_size) {
    edge_bus_datahub_port_t port = edge_bus_datahub_port(hub);
    return bus_orchestrator_create(&port, arb, driver_port, max_buses, pool_size, buf_size);
}
