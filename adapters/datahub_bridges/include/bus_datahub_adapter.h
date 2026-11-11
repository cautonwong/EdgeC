#ifndef BUS_DATAHUB_ADAPTER_H
#define BUS_DATAHUB_ADAPTER_H

#include "bus_orchestrator.h"
#include "datahub.h"

edge_bus_datahub_port_t edge_bus_datahub_port(datahub_t *hub);

bus_orchestrator_t *bus_orchestrator_create_hub(bus_orchestrator_storage_t *storage, datahub_t *hub,
                                                arbiter_t *arb,
                                                const edge_driver_session_port_t *driver_port,
                                                int max_buses, uint32_t pool_size, uint32_t buf_size);

bus_orchestrator_t *bus_orchestrator_create_hub_default(datahub_t *hub, arbiter_t *arb,
                                                        const edge_driver_session_port_t *driver_port,
                                                        int max_buses, uint32_t pool_size,
                                                        uint32_t buf_size);

#endif /* BUS_DATAHUB_ADAPTER_H */
