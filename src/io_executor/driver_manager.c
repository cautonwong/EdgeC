#include "driver_manager.h"

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define MAX_DRIVERS 32

struct driver_manager_s {
    edge_driver_t *drivers[MAX_DRIVERS];
    int count;
};

driver_manager_t *driver_manager_create(void) {
    driver_manager_t *mgr = calloc(1, sizeof(struct driver_manager_s));
    return mgr;
}

void driver_manager_destroy(driver_manager_t *mgr) {
    if (mgr) free(mgr);
}

int driver_manager_register(driver_manager_t *mgr, edge_driver_t *driver) {
    if (!mgr || !driver) return -1;
    if (mgr->count >= MAX_DRIVERS) return -2;

    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->drivers[i]->name, driver->name) == 0) return 0;
    }

    mgr->drivers[mgr->count++] = driver;
    LOG_INFO(LOG_CAT_IO, "Registered Protocol Template: %s", driver->name);
    return 0;
}

edge_driver_t *driver_manager_find(driver_manager_t *mgr, const char *name) {
    if (!mgr || !name) return NULL;
    for (int i = 0; i < mgr->count; i++) {
        if (strcmp(mgr->drivers[i]->name, name) == 0) return mgr->drivers[i];
    }
    return NULL;
}

edge_driver_ctx_t *driver_instance_create(driver_manager_t *mgr, const char *protocol,
                                          uint32_t device_id) {
    edge_driver_t *tmpl = driver_manager_find(mgr, protocol);
    if (!tmpl) return NULL;

    edge_driver_ctx_t *ctx = calloc(1, sizeof(edge_driver_ctx_t));
    ctx->driver = tmpl;
    ctx->device_id = device_id;
    if (tmpl->ctx_size > 0) ctx->priv = calloc(1, tmpl->ctx_size);
    if (tmpl->on_init) tmpl->on_init(ctx);

    return ctx;
}

void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol) {
    (void)protocol;
    if (!ctx) return;
    if (ctx->driver && ctx->driver->on_cleanup) ctx->driver->on_cleanup(ctx);
    if (ctx->priv) free(ctx->priv);
    free(ctx);
}
