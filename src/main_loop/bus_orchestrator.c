#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"
#include "shmring.h"

struct bus_orchestrator_s {
    edge_driver_t *driver;
    datahub_t *hub;
    int is_running;
};

bus_orchestrator_t *bus_orchestrator_create(edge_driver_t *driver, datahub_t *hub) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->driver = driver;
    ctx->hub = hub;
    return ctx;
}

static void on_io_complete(bus_orchestrator_t *ctx, edge_point_config_t *point, uint8_t *rx_buf,
                           size_t rx_len) {
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));

    if (ctx->driver->parse_response(point, rx_buf, rx_len, &msg) == 0) {
        // 使用 DataHub 标准发布接口
        datahub_publish(ctx->hub, &msg);
    }
}

void bus_orchestrator_run(bus_orchestrator_t *ctx) {
    ctx->is_running = 1;
    LOG_INFO(LOG_CAT_BUS, "Orchestrator plane active");
    (void)on_io_complete;
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (ctx) free(ctx);
}
