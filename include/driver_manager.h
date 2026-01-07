#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H

#include <stdint.h>

#include "edge_driver.h"

typedef struct driver_manager_s driver_manager_t;

/**
 * @brief 创建驱动管理器实例 (组合模式)
 * 替代旧版的全局初始化
 */
driver_manager_t *driver_manager_create(void);

/**
 * @brief 销毁管理器
 */
void driver_manager_destroy(driver_manager_t *mgr);

/**
 * @brief 注册协议模板到指定管理器
 */
int driver_manager_register(driver_manager_t *mgr, edge_driver_t *driver);

/**
 * @brief 查找协议模板
 */
edge_driver_t *driver_manager_find(driver_manager_t *mgr, const char *name);

/**
 * @brief 实例化驱动上下文 (Stateful V3)
 */
edge_driver_ctx_t *driver_instance_create(driver_manager_t *mgr, const char *protocol,
                                          uint32_t device_id);

/**
 * @brief 销毁驱动实例
 */
void driver_instance_destroy(edge_driver_ctx_t *ctx, const char *protocol);

#endif