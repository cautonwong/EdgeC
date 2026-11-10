/**
 * @file edge_io_guard_port.h
 * @brief Consumer-owned command/telemetry guard port for edge_io engines.
 */
#ifndef EDGE_IO_GUARD_PORT_H
#define EDGE_IO_GUARD_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*validate_telemetry)(void *ctx, uint32_t tag_id, double value);
} edge_io_guard_port_t;

void edge_io_set_guard_port(const edge_io_guard_port_t *port);
const edge_io_guard_port_t *edge_io_get_guard_port(void);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_IO_GUARD_PORT_H */
