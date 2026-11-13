#include "edge_thread_port.h"

#include <stddef.h>

static edge_thread_port_t g_thread_port;

int edge_thread_port_install(const edge_thread_port_t *port) {
    if (!port || !port->create || !port->join) return -1;
    g_thread_port = *port;
    return 0;
}

int edge_thread_create(void *storage, void (*entry)(void *), void *arg) {
    if (!g_thread_port.create || !storage || !entry) return -1;
    return g_thread_port.create(g_thread_port.ctx, storage, entry, arg);
}

int edge_thread_join(void *storage) {
    if (!g_thread_port.join || !storage) return -1;
    return g_thread_port.join(g_thread_port.ctx, storage);
}
