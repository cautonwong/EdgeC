#ifndef EDGE_DATAHUB_WRITE_PORT_H
#define EDGE_DATAHUB_WRITE_PORT_H

#include "pipe_msg.h"

/** Minimal DataHub write port for producers that only publish shadow values. */
typedef struct {
    void *ctx;
    int (*write_shadow)(void *ctx, const PipeMsg *msg);
} edge_datahub_write_port_t;

#endif /* EDGE_DATAHUB_WRITE_PORT_H */
