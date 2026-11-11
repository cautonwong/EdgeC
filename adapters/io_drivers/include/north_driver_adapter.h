#ifndef NORTH_DRIVER_ADAPTER_H
#define NORTH_DRIVER_ADAPTER_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "north_driver_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NORTH_DRIVER_ADAPTER_STORAGE_BYTES 70000
#define NORTH_DRIVER_FRAME_BYTES 2048

typedef struct {
    alignas(max_align_t) uint8_t raw[NORTH_DRIVER_ADAPTER_STORAGE_BYTES];
} north_driver_adapter_storage_t;

/* Caller-allocated adapter. The returned port copies ctx and must be stored
 * in the northbound manager before it starts. */
north_driver_port_t north_driver_adapter_init(north_driver_adapter_storage_t *storage);

#ifdef __cplusplus
}
#endif

#endif /* NORTH_DRIVER_ADAPTER_H */
