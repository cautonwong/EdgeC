#ifndef STREAM_BUS_DATAHUB_ADAPTER_H
#define STREAM_BUS_DATAHUB_ADAPTER_H

#include "stream_bus_port.h"
#include "stream_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/*
 * Adapter at the composition side. edge_stream sees only the port; this is the
 * one place that knows the bus is datahub.
 */
stream_bus_port_t stream_bus_datahub_port(datahub_t *hub);

stream_engine_t *stream_engine_create_datahub(datahub_t *hub, uv_loop_t *loop, void *orch,
                                               const edge_stage_factory_entry_t *registry);

stream_engine_t *stream_engine_create_in_datahub(stream_engine_storage_t *storage, datahub_t *hub,
                                                  uv_loop_t *loop, void *orch,
                                                  const edge_stage_factory_entry_t *registry);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_BUS_DATAHUB_ADAPTER_H */
