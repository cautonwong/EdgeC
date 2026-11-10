#include "js_arbiter_service_adapter.h"

#include "edge_service.h"

static int _get_statuses(void *ctx, arbiter_bus_status_t *out, int max_count) {
    arbiter_t *arb = service_get_arbiter((service_t *)ctx);
    return arb ? arbiter_get_statuses(arb, out, max_count) : 0;
}

static int _submit(void *ctx, int bus_id, cmd_msg_t *cmd) {
    arbiter_t *arb = service_get_arbiter((service_t *)ctx);
    return arb ? arbiter_submit_cmd(arb, bus_id, cmd) : -1;
}

js_arbiter_port_t js_arbiter_port_from_service(service_t *svc) {
    js_arbiter_port_t port = {
        .ctx = svc,
        .get_statuses = _get_statuses,
        .submit = _submit,
    };
    return port;
}
