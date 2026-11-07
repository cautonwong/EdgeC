#include "radar_datahub_adapter.h"

#include "datahub.h"

static int _write_shadow(void *ctx, const pipe_msg_t *msg) {
    return datahub_write_shadow((datahub_t *)ctx, msg);
}

radar_bus_port_t radar_datahub_port(datahub_t *hub) {
    radar_bus_port_t port = {.ctx = hub, .write_shadow = _write_shadow};
    return port;
}
