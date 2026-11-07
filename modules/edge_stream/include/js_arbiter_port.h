#ifndef JS_ARBITER_PORT_H
#define JS_ARBITER_PORT_H

#include "arbiter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*get_statuses)(void *ctx, arbiter_bus_status_t *out, int max_count);
    int (*submit)(void *ctx, int bus_id, cmd_msg_t *cmd);
} js_arbiter_port_t;

static inline int js_arbiter_port_get_statuses(const js_arbiter_port_t *port,
                                          arbiter_bus_status_t *out, int max_count) {
    return (port && port->get_statuses) ? port->get_statuses(port->ctx, out, max_count) : 0;
}

static inline int js_arbiter_port_submit(const js_arbiter_port_t *port, int bus_id, cmd_msg_t *cmd) {
    return (port && port->submit) ? port->submit(port->ctx, bus_id, cmd) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* JS_ARBITER_PORT_H */
