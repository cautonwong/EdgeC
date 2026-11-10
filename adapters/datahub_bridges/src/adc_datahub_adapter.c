#include "adc_datahub_adapter.h"

#include "datahub.h"

static int _publish(void *ctx, const pipe_msg_t *msg) {
    return datahub_publish((datahub_t *)ctx, msg);
}

static int _register_ref(void *ctx, uint32_t tag_id, int fd, size_t len) {
    return datahub_register_ref((datahub_t *)ctx, tag_id, fd, len);
}

adc_bus_port_t adc_datahub_port(datahub_t *hub) {
    adc_bus_port_t port = {.ctx = hub, .publish = _publish, .register_ref = _register_ref};
    return port;
}
