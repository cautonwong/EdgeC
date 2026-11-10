#include "edge_io_guard_port.h"

static const edge_io_guard_port_t *g_guard;

void edge_io_set_guard_port(const edge_io_guard_port_t *port) {
    g_guard = port;
}

const edge_io_guard_port_t *edge_io_get_guard_port(void) {
    return g_guard;
}
