#ifndef STREAM_BUS_PORT_H
#define STREAM_BUS_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The stream engine's outbound view of the data bus. The component reads and
 * publishes pipe messages; it does not know whether the bus is shared memory,
 * DDS, or a test double.
 */
typedef struct {
    void *ctx;
    int (*publish)(void *ctx, const pipe_msg_t *msg);
    int (*write_shadow)(void *ctx, const pipe_msg_t *msg);
    int (*read_shadow)(void *ctx, uint32_t tag_id, pipe_msg_t *out);
    int (*read_ring)(void *ctx, uint64_t *seq, pipe_msg_t *out);
    uint64_t (*ring_seq)(void *ctx);
    int (*add_listener)(void *ctx, uv_async_t *handle);
    int (*remove_listener)(void *ctx, uv_async_t *handle);
} stream_bus_port_t;

static inline int stream_bus_publish(const stream_bus_port_t *bus, const pipe_msg_t *msg) {
    return (bus && bus->publish) ? bus->publish(bus->ctx, msg) : -1;
}

static inline int stream_bus_write_shadow(const stream_bus_port_t *bus, const pipe_msg_t *msg) {
    return (bus && bus->write_shadow) ? bus->write_shadow(bus->ctx, msg) : -1;
}

static inline int stream_bus_read_shadow(const stream_bus_port_t *bus, uint32_t tag_id,
                                         pipe_msg_t *out) {
    return (bus && bus->read_shadow) ? bus->read_shadow(bus->ctx, tag_id, out) : -1;
}

static inline int stream_bus_read_ring(const stream_bus_port_t *bus, uint64_t *seq,
                                       pipe_msg_t *out) {
    return (bus && bus->read_ring) ? bus->read_ring(bus->ctx, seq, out) : -1;
}

static inline uint64_t stream_bus_ring_seq(const stream_bus_port_t *bus) {
    return (bus && bus->ring_seq) ? bus->ring_seq(bus->ctx) : 0;
}

static inline int stream_bus_add_listener(const stream_bus_port_t *bus, uv_async_t *handle) {
    return (bus && bus->add_listener) ? bus->add_listener(bus->ctx, handle) : -1;
}

static inline int stream_bus_remove_listener(const stream_bus_port_t *bus, uv_async_t *handle) {
    return (bus && bus->remove_listener) ? bus->remove_listener(bus->ctx, handle) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* STREAM_BUS_PORT_H */
