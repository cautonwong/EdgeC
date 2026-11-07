#ifndef PROTOCOL_BRIDGE_BUS_PORT_H
#define PROTOCOL_BRIDGE_BUS_PORT_H

#include <stdint.h>
#include <uv.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*read_ring)(void *ctx, uint64_t *seq, pipe_msg_t *out);
    int (*add_listener)(void *ctx, uv_async_t *handle);
    int (*remove_listener)(void *ctx, uv_async_t *handle);
} protocol_bridge_bus_port_t;

static inline int protocol_bridge_bus_read_ring(const protocol_bridge_bus_port_t *bus,
                                                uint64_t *seq, pipe_msg_t *out) {
    return (bus && bus->read_ring) ? bus->read_ring(bus->ctx, seq, out) : -1;
}

static inline int protocol_bridge_bus_add_listener(const protocol_bridge_bus_port_t *bus,
                                                   uv_async_t *handle) {
    return (bus && bus->add_listener) ? bus->add_listener(bus->ctx, handle) : -1;
}

static inline int protocol_bridge_bus_remove_listener(const protocol_bridge_bus_port_t *bus,
                                                      uv_async_t *handle) {
    return (bus && bus->remove_listener) ? bus->remove_listener(bus->ctx, handle) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_BRIDGE_BUS_PORT_H */
