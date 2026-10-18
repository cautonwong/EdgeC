/**
 * @file iems_driver_adapter.h
 * @brief IEMS 专属南向工业协议驱动防腐适配层与静态点表注册 (No-Contract-Compiler)
 */

#ifndef IEMS_DRIVER_ADAPTER_H
#define IEMS_DRIVER_ADAPTER_H

#include <stdint.h>
#include <stddef.h>

#include "driver_manager.h"
#include "edge_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EDGE_SHM_BUS_NAME
#define EDGE_SHM_BUS_NAME "iems"
#endif

#ifndef EDGE_UI_SOCKET_PATH
#define EDGE_UI_SOCKET_PATH "/tmp/iems_ui.sock"
#endif

/* IEMS 标准核心测点 ID 常量定义 (纯本地有界上下文枚举) */
#define IEMS_TAG_VOLTAGE_PHASE_A     1001
#define IEMS_TAG_CURRENT_PHASE_A     1002
#define IEMS_TAG_GRID_POWER          1003
#define IEMS_TAG_GRID_FREQUENCY      1004
#define IEMS_TAG_ADC_RAW_DATA        2001
#define IEMS_TAG_ADC_VOLTAGE_RMS     2002
#define IEMS_TAG_ENERGY_ACTIVE_IMP   3001
#define IEMS_TAG_BATTERY_SOC         4001
#define IEMS_TAG_BATTERY_CURRENT     4002
#define IEMS_TAG_BATTERY_POWER       4003
#define IEMS_TAG_SOLAR_POWER         8001
#define IEMS_TAG_SOLAR_VOLTS         8003
#define IEMS_TAG_SOLAR_RELAY         8004
#define IEMS_TAG_PCS_CONTROL         8005

/**
 * @brief 初始化并注册 IEMS 储能系统所需的工业协议驱动模板 (ModbusRTU, IEC104, CAN, DLT645)
 */
int iems_driver_registry_init(driver_manager_t *mgr);

/**
 * @brief 获取 IEMS 针对特定协议的测点映射注入包 (替代全局 contract_model_find)
 */
const driver_contract_injection_t *iems_get_driver_injection(const char *proto);

/**
 * @brief IEMS 本地测点到 DataHub 影子槽位的 O(1) 稠密映射函数
 */
int iems_tag_map_fn(uint32_t tag_id, uint32_t device_id);

/**
 * @brief 获取测点元数据默认轮询周期 (ms)
 */
uint32_t iems_get_tag_poll_interval_ms(uint32_t tag_id);

#include "contract_data_types.h"

/**
 * @brief 获取 IEMS 物理总线通道静态配置表
 */
const edge_connectivity_t *iems_get_connectivity(uint32_t *count);

/**
 * @brief 获取 IEMS 数据流处理管道静态配置表
 */
const edge_pipeline_def_t *iems_get_pipelines(uint32_t *count);

/**
 * @brief 获取 IEMS 测点静态元数据表
 */
const edge_tag_meta_t *iems_get_tag_metadata(uint32_t *count);
const edge_tag_meta_t *iems_get_tag_meta(uint32_t tag_id);

/* 兼容旧式全局符号与测试链接 */
extern const edge_tag_meta_t *g_tag_metadata;
extern const size_t g_tag_metadata_len;
extern const edge_connectivity_t *g_connectivity_configs;
extern const int g_connectivity_configs_len;
extern const edge_pipeline_def_t *g_pipeline_metadata;
extern const int g_pipeline_metadata_len;

const char *edge_resolve_bus_path(int bus_id);
int edge_tag_get_compact_index(uint32_t tag_id);
const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t tag_id);
const edge_stage_factory_entry_t *edge_stage_factory_get_registry(void);

/* 兼容旧式 TAG 符号与全局查找接口 */
#define TAG_VOLTAGE_PHASE_A       IEMS_TAG_VOLTAGE_PHASE_A
#define TAG_ENERGY_ACTIVE_IMPORT  IEMS_TAG_ENERGY_ACTIVE_IMP

edge_tag_id_t edge_lookup_modbus(uint64_t addr, float *scale);
edge_tag_id_t edge_lookup_dlt645(uint64_t di, float *scale);
int edge_contract_self_test(void);
int edge_driver_registry_init(driver_manager_t *mgr);
const driver_contract_injection_t *edge_model_find(const char *name);

extern const edge_driver_reg_entry_t *g_edge_driver_registry;
extern const size_t g_edge_driver_registry_count;

/* 兼容外部全栈测试点表反查符号导出 */
const void *iems_lookup_modbus_by_key(uint64_t key);
const void *iems_lookup_modbus_by_tag(uint32_t tag_id);
const void *iems_lookup_iec104_by_key(uint64_t key);
const void *iems_lookup_iec104_by_tag(uint32_t tag_id);

const modbus_map_t *edge_get_map_modbus(uint64_t key);
const modbus_map_t *edge_get_map_modbus_by_tag(edge_tag_id_t tag_id);
const iec104_map_t *edge_get_map_iec104(uint64_t key);
const iec104_map_t *edge_get_map_iec104_by_tag(edge_tag_id_t tag_id);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_DRIVER_ADAPTER_H
