#ifndef NORTH_DRIVER_TEST_PORT_H
#define NORTH_DRIVER_TEST_PORT_H

#include "north_driver_adapter.h"

/* Each test translation unit gets its own caller-allocated adapter storage. */
static north_driver_adapter_storage_t g_test_north_driver_storage;

static inline north_driver_port_t test_north_driver_port(void) {
    return north_driver_adapter_init(&g_test_north_driver_storage);
}

#endif /* NORTH_DRIVER_TEST_PORT_H */
