#ifndef RADAR_BUS_PORT_H
#define RADAR_BUS_PORT_H

#include <stdint.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*write_shadow)(void *ctx, const pipe_msg_t *msg);
} radar_bus_port_t;

static inline int radar_bus_write_shadow(const radar_bus_port_t *bus, const pipe_msg_t *msg) {
    return (bus && bus->write_shadow) ? bus->write_shadow(bus->ctx, msg) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* RADAR_BUS_PORT_H */
