#ifndef EDGE_DRIVER_REGISTRY_PORT_H
#define EDGE_DRIVER_REGISTRY_PORT_H

#include <stddef.h>

#include "edge_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 驱动注册端口。
 *
 * 产品适配器只负责提供静态注册表，不依赖具体 driver_manager 实现。
 */
typedef struct {
    void *ctx;
    int (*register_driver)(void *ctx, const edge_driver_t *driver);
    int (*register_table)(void *ctx, const edge_driver_reg_entry_t *table, size_t count);
} edge_driver_registry_port_t;

#ifdef __cplusplus
}
#endif

#endif
