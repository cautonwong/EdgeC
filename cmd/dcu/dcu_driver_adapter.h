/**
 * @file dcu_driver_adapter.h
 * @brief DCU 专属南向驱动防腐适配层与静态点表注册 (No-Contract-Compiler PoC)
 */

#ifndef DCU_DRIVER_ADAPTER_H
#define DCU_DRIVER_ADAPTER_H

#include "dcu_inventory.h"
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

/** @brief 注入仪表档案，让 DLT645 驱动按 device_id 取真实表地址（不再是广播 0x99） */
void dcu_driver_adapter_set_meters(const production_meter_t *meters, uint32_t count);

/**
 * @brief 获取 DCU 针对特定协议的测点映射注入包 (替代全局 contract_model_find)
 *
 * @param proto 协议名称 (如 "DLT698", "DLT645", "ModbusRTU")
 * @return 对应的驱动依赖注入句柄指针，若无匹配则返回 NULL
 */
const driver_contract_injection_t *dcu_get_driver_injection(const char *proto);

/**
 * @brief 消费侧 OAD → 线上协议号 (DLT645 的 DI / Modbus 的寄存器号)
 *
 * 映射归 cmd 层：驱动只发/收协议号，不认识 OAD；找不到返回 0。
 */
uint32_t dcu_driver_adapter_oad_to_proto(const char *proto, uint32_t oad);

/** @brief 线上协议号 → 消费侧 OAD；找不到返回 0 */
uint32_t dcu_driver_adapter_proto_to_oad(const char *proto, uint32_t proto_id);

/** @brief 协议号对应的量纲倍数 (scale)；找不到返回 0.0 */
double dcu_driver_adapter_scale(const char *proto, uint32_t proto_id);

#ifdef __cplusplus
}
#endif

#endif  // DCU_DRIVER_ADAPTER_H
