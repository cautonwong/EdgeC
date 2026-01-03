#include "driver_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

// --- 全局驱动元数据表 ---
static edge_driver_t* g_protocol_templates[32];
static int g_template_count = 0;

int driver_manager_init(void) {
    g_template_count = 0;
    return 0;
}

int driver_manager_register(edge_driver_t* driver) {
    if (!driver || g_template_count >= 32) return -1;
    g_protocol_templates[g_template_count++] = driver;
    LOG_INFO(LOG_CAT_DRIVER, "Registered Protocol Template: %s", driver->name);
    return 0;
}

edge_driver_t* driver_manager_find(const char* name) {
    for (int i = 0; i < g_template_count; i++) {
        if (strcmp(g_protocol_templates[i]->name, name) == 0) return g_protocol_templates[i];
    }
    return NULL;
}

// --- 运行时实例分配 (V3 核心) ---

edge_driver_ctx_t* driver_instance_create(const char* protocol_name, uint32_t device_id) {
    edge_driver_t* drv = driver_manager_find(protocol_name);
    if (!drv) {
        LOG_ERROR(LOG_CAT_DRIVER, "Cannot find protocol template for: %s", protocol_name);
        return NULL;
    }

    // 分配上下文包装器
    edge_driver_ctx_t* ctx = calloc(1, sizeof(edge_driver_ctx_t));
    ctx->driver = drv;  // [填充模板指针]
    ctx->device_id = device_id;

    // 物理分配协议私有内存
    if (drv->ctx_size > 0) {
        ctx->priv = calloc(1, drv->ctx_size);
    }

    // 初始化协议状态
    if (drv->on_init) {
        if (drv->on_init(ctx) != 0) {
            LOG_ERROR(LOG_CAT_DRIVER, "Failed to init protocol state for: %s", protocol_name);
            free(ctx->priv);
            free(ctx);
            return NULL;
        }
    }

    LOG_INFO(LOG_CAT_DRIVER, "Instantiated Protocol %s for Device %u", protocol_name, device_id);
    return ctx;
}

void driver_instance_destroy(edge_driver_ctx_t* ctx, const char* protocol_name) {
    if (!ctx) return;

    edge_driver_t* drv = driver_manager_find(protocol_name);
    if (drv && drv->on_cleanup) {
        drv->on_cleanup(ctx);
    }

    if (ctx->priv) free(ctx->priv);
    free(ctx);
}

void driver_manager_fini(void) {
    g_template_count = 0;
}
