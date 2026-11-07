#include "edge_mcp_datahub_adapter.h"

#include "datahub.h"

static int _read_shadow(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    return datahub_read_shadow((datahub_t *)ctx, tag_id, out);
}

static int _write_shadow(void *ctx, const pipe_msg_t *msg) {
    return datahub_write_shadow((datahub_t *)ctx, msg);
}

edge_mcp_bus_port_t edge_mcp_datahub_port(datahub_t *hub) {
    edge_mcp_bus_port_t port = {.ctx = hub, .read_shadow = _read_shadow, .write_shadow = _write_shadow};
    return port;
}
