#ifndef EDGE_BUS_DATAHUB_PORT_H
#define EDGE_BUS_DATAHUB_PORT_H

#include "pipe_msg.h"

typedef struct uv_loop_s uv_loop_t;

/** Consumer-defined DataHub access for the bus orchestrator. */
typedef struct {
    void *ctx;
    uv_loop_t *(*get_loop)(void *ctx);
    int (*publish)(void *ctx, const PipeMsg *msg);
} edge_bus_datahub_port_t;

#endif /* EDGE_BUS_DATAHUB_PORT_H */
