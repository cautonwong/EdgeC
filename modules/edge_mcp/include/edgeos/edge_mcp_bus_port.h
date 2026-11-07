#ifndef EDGE_MCP_BUS_PORT_H
#define EDGE_MCP_BUS_PORT_H

#include <stdint.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*read_shadow)(void *ctx, uint32_t tag_id, pipe_msg_t *out);
    int (*write_shadow)(void *ctx, const pipe_msg_t *msg);
} edge_mcp_bus_port_t;

static inline int edge_mcp_bus_read_shadow(const edge_mcp_bus_port_t *bus, uint32_t tag_id,
                                           pipe_msg_t *out) {
    return (bus && bus->read_shadow) ? bus->read_shadow(bus->ctx, tag_id, out) : -1;
}

static inline int edge_mcp_bus_write_shadow(const edge_mcp_bus_port_t *bus, const pipe_msg_t *msg) {
    return (bus && bus->write_shadow) ? bus->write_shadow(bus->ctx, msg) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_BUS_PORT_H */
