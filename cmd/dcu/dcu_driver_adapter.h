/**
 * @file dcu_driver_adapter.h
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册 (No-Contract-Compiler PoC)
 */

#ifndef DCU_DRIVER_ADAPTER_H
#define DCU_DRIVER_ADAPTER_H

#include "driver_manager.h"
#include "edge_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化并注册 DCU 所需的工业协议驱动模板 (DLT698, DLT645, ModbusRTU)
 *
 * @param mgr 驱动管理器实例
 * @return 0 成功, -1 失败
 */
int dcu_driver_registry_init(driver_manager_t *mgr);

/**
 * @brief 获取 DCU 针对特定协议的测点映射注入包 (替代全局 contract_model_find)
 *
 * @param proto 协议名称 (如 "DLT698", "DLT645", "ModbusRTU")
 * @return 对应的驱动依赖注入句柄指针，若无匹配则返回 NULL
 */
const driver_contract_injection_t *dcu_get_driver_injection(const char *proto);

#ifdef __cplusplus
}
#endif

#endif  // DCU_DRIVER_ADAPTER_H
