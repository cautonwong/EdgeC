/**
 * @file driver_manager.c
 * @brief 驱动工厂管理器 - 负责协议模板的生命周期与实例映射
 */

#include "driver_manager.h"

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define MAX_DRIVERS 32

struct driver_manager_s {
    edge_driver_t *drivers[MAX_DRIVERS];
    int count;
};

static struct driver_manager_s *g_manager = NULL;

// [驱动注册宏集]
extern int modbus_rtu_driver_register(void);
extern int dlt645_driver_register(void);
extern int dlms_driver_register(void);

int driver_manager_init(void) {
    if (g_manager) return 0;
    g_manager = calloc(1, sizeof(struct driver_manager_s));
    if (!g_manager) return -1;

    // 自动注册核心驱动，实现“开箱即用”
    modbus_rtu_driver_register();
    dlt645_driver_register();
    dlms_driver_register();

    return 0;
}

int driver_manager_register(edge_driver_t *driver) {
    if (!g_manager || !driver) return -1;
    if (g_manager->count >= MAX_DRIVERS) return -2;

    // 检查重复
    for (int i = 0; i < g_manager->count; i++) {
        if (strcmp(g_manager->drivers[i]->name, driver->name) == 0) return 0;
    }

    g_manager->drivers[g_manager->count++] = driver;
    LOG_INFO(LOG_CAT_IO, "Registered Protocol Template: %s", driver->name);
    return 0;
}

edge_driver_t *driver_manager_find(const char *name) {
    if (!g_manager || !name) return NULL;
    for (int i = 0; i < g_manager->count; i++) {
        if (strcmp(g_manager->drivers[i]->name, name) == 0) return g_manager->drivers[i];
    }
    return NULL;
}

edge_driver_ctx_t *driver_instance_create(const char *protocol, uint32_t device_id) {
    edge_driver_t *tmpl = driver_manager_find(protocol);
    if (!tmpl) {
        LOG_ERROR(LOG_CAT_IO, "Cannot find protocol template for: %s", protocol);
        return NULL;
    }

    edge_driver_ctx_t *ctx = calloc(1, sizeof(edge_driver_ctx_t));
    ctx->driver = tmpl;
    ctx->device_id = device_id;
    if (tmpl->ctx_size > 0) {
        ctx->priv = calloc(1, tmpl->ctx_size);
    }

    if (tmpl->on_init) tmpl->on_init(ctx);

    LOG_INFO(LOG_CAT_IO, "Instantiated Protocol %s for Device %u", protocol, device_id);
    return ctx;
}

void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol) {
    if (!ctx) return;
    edge_driver_t *tmpl = driver_manager_find(protocol);
    if (tmpl && tmpl->on_cleanup) tmpl->on_cleanup(ctx);
    if (ctx->priv) free(ctx->priv);
    free(ctx);
}

void driver_manager_fini(void) {
    if (g_manager) {
        free(g_manager);
        g_manager = NULL;
    }
}