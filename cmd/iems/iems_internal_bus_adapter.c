#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "adc_datahub_adapter.h"
#include "adc_driver.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "iems_internal_bus_adapter.h"

typedef struct {
    int fd;
    adc_bus_port_t bus;
} adc_thread_ctx_t;

volatile bool g_adc_running = false;

static void _adc_thread_entry(void *arg) {
    adc_thread_ctx_t *ctx = (adc_thread_ctx_t *)arg;
    adc_driver_handle_connection(ctx->fd, &ctx->bus);
    g_adc_running = false;
    free(ctx);
}

static void _try_connect_internal_buses(uv_timer_t *handle) {
    iems_internal_bus_t *ibus = (iems_internal_bus_t *)handle->data;
    if (!ibus || g_adc_running) return;

    uint32_t conn_count = 0;
    const edge_connectivity_t *conns = iems_get_connectivity(&conn_count);
    if (!conns) return;

    for (uint32_t i = 0; i < conn_count; i++) {
        const edge_connectivity_t *c = &conns[i];
        if (strcmp(c->category, "INTERNAL") != 0 || g_adc_running) continue;
        const char *path = edge_resolve_bus_path(c->id);
        if (path && access(path, F_OK) == 0) {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) continue;
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                adc_thread_ctx_t *ctx = malloc(sizeof(adc_thread_ctx_t));
                ctx->fd = fd;
                ctx->bus = adc_datahub_port(ibus->hub);
                g_adc_running = true;
                uv_thread_create(&ibus->tid, _adc_thread_entry, ctx);
                ibus->thread_started = true;
            } else {
                close(fd);
            }
        }
    }
}

void iems_internal_bus_start(iems_internal_bus_t *ibus, uv_loop_t *loop, datahub_t *hub) {
    if (!ibus || !loop) return;
    ibus->loop = loop;
    ibus->hub = hub;
    uv_timer_init(loop, &ibus->timer);
    ibus->timer.data = ibus;
    ibus->timer_initialized = true;
    uv_timer_start(&ibus->timer, _try_connect_internal_buses, 0, 500);
}

void iems_internal_bus_stop(iems_internal_bus_t *ibus) {
    if (!ibus || !ibus->timer_initialized) return;
    uv_timer_stop(&ibus->timer);
    uv_close((uv_handle_t *)&ibus->timer, NULL);
    ibus->timer_initialized = false;
}

void iems_internal_bus_join(iems_internal_bus_t *ibus) {
    if (!ibus || !ibus->thread_started) return;
    g_adc_running = false;
    uv_thread_join(&ibus->tid);
    ibus->thread_started = false;
}
