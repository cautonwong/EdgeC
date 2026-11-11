#include "datahub_write_adapter.h"

static int _write_shadow(void *ctx, const PipeMsg *msg) {
    return datahub_write_shadow((datahub_t *)ctx, msg);
}

edge_datahub_write_port_t edge_datahub_write_port(datahub_t *hub) {
    edge_datahub_write_port_t port = {.ctx = hub, .write_shadow = _write_shadow};
    return port;
}
