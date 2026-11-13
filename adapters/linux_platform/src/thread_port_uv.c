#include "thread_port_uv.h"

#include <uv.h>

static int _uv_thread_port_create(void *ctx, void *storage, void (*entry)(void *), void *arg) {
    (void)ctx;
    return uv_thread_create((uv_thread_t *)storage, (uv_thread_cb)entry, arg);
}

static int _uv_thread_port_join(void *ctx, void *storage) {
    (void)ctx;
    return uv_thread_join((uv_thread_t *)storage);
}

const edge_thread_port_t g_uv_thread_port = {
    .ctx = NULL,
    .create = _uv_thread_port_create,
    .join = _uv_thread_port_join,
};

__attribute__((constructor)) static void _install_uv_thread_port(void) {
    edge_thread_port_install(&g_uv_thread_port);
}

void edge_thread_port_install_uv(void) {
    edge_thread_port_install(&g_uv_thread_port);
}
