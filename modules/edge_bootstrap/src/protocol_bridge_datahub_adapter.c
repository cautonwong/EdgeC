#include "protocol_bridge_datahub_adapter.h"

#include "datahub.h"

static int _read_ring(void *ctx, uint64_t *seq, pipe_msg_t *out) {
    return datahub_read_ring((datahub_t *)ctx, seq, out);
}

static int _add_listener(void *ctx, uv_async_t *handle) {
    return datahub_add_listener((datahub_t *)ctx, handle);
}

static int _remove_listener(void *ctx, uv_async_t *handle) {
    return datahub_remove_listener((datahub_t *)ctx, handle);
}

protocol_bridge_bus_port_t protocol_bridge_datahub_port(datahub_t *hub) {
    protocol_bridge_bus_port_t port = {
        .ctx = hub,
        .read_ring = _read_ring,
        .add_listener = _add_listener,
        .remove_listener = _remove_listener,
    };
    return port;
}
