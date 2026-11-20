/**
 * @file iems_driver_adapter.h
 * @brief IEMS 专属南向工业协议驱动防腐适配层与静态点表注册 (No-Contract-Compiler)
 */

#ifndef IEMS_DRIVER_ADAPTER_H
#define IEMS_DRIVER_ADAPTER_H

#include <stdint.h>
#include <stddef.h>

#include "edge_driver_registry_port.h"
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
int iems_driver_registry_init(const edge_driver_registry_port_t *registry);

/**
 * @brief 把库存协议串归一化为驱动注册表里的规范名
 * @param raw 数据库原始值，可为 NULL
 * @param out 至少 16 字节，仅成功时写入
 * @return 0 成功；-1 表示空值、未知值或仅支持 1997 的 DLT645
 */
int iems_driver_protocol_canonical(const char *raw, char out[16]);

/* [ADR-002] 点位标识翻译与量纲：产品侧负责，驱动不再查表。
 *   native_id：tag_id → 驱动认识的原生标识（CAN 是 CAN ID，其余暂等值）
 *   tag_scale：该点位的工程量系数，来自点表 scaler；Modbus 家族统一
 *   raw×scaler。读路径乘它、写路径除以它，两者互逆，TCP/RTU 不按协议分叉。 */
uint32_t iems_driver_adapter_native_id(const char *proto, uint32_t tag_id);
double iems_driver_adapter_tag_scale(const char *proto, uint32_t tag_id);

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
int edge_driver_registry_init(const edge_driver_registry_port_t *registry);

extern const edge_driver_reg_entry_t *g_edge_driver_registry;
extern const size_t g_edge_driver_registry_count;

/* 兼容外部全栈测试点表反查符号导出 */
const void *iems_lookup_modbus_by_key(uint64_t key);
const void *iems_lookup_modbus_by_tag(uint32_t tag_id);
const void *iems_lookup_iec104_by_key(uint64_t key);
const void *iems_lookup_iec104_by_tag(uint32_t tag_id);

const modbus_map_t *edge_get_map_modbus(uint64_t key);
const modbus_map_t *edge_get_map_modbus_by_tag(edge_tag_id_t tag_id);
const dlt645_map_t *iems_lookup_dlt645_by_tag(uint32_t tag_id);
const iec104_map_t *edge_get_map_iec104(uint64_t key);
const iec104_map_t *edge_get_map_iec104_by_tag(edge_tag_id_t tag_id);
/* 静态 IEC104 点表（IOA 16385..16388）；总召唤按此枚举。 */
const iec104_map_t *iems_get_iec104_map(size_t *count);

/* v3 的遥控/遥调地址分段（iems_v3/src/automatic_north/north_automatic.h:38-41、
 * iec104_create.c:64-67）：
 *   RemoteCtrl = 24577 + 20*section + n
 *   RemoteRegu = 25089 + 500*section + n
 * section 是设备分段号。具体点表由现场 xlsx 生成，仓库没有该表；在补表之前
 * 所有控制 IOA 都按未映射拒绝（COT=47）。 */
#define IEC104_REMOTE_CTRL_BASE 24577u
#define IEC104_REMOTE_CTRL_RESERVE 20u
#define IEC104_REMOTE_REGU_BASE 25089u
#define IEC104_REMOTE_REGU_RESERVE 500u

typedef enum {
    IEC104_CONTROL_SPACE_NONE = 0,
    IEC104_CONTROL_SPACE_REMOTE_CTRL,
    IEC104_CONTROL_SPACE_REMOTE_REGU,
} iec104_control_space_t;

/* 控制点位：IOA → 产品 tag。补表时逐点登记。 */
typedef struct {
    uint32_t ioa;
    edge_tag_id_t tag_id;
} iec104_control_map_t;

iec104_control_space_t iems_iec104_control_space(uint32_t ioa);
const iec104_control_map_t *iems_lookup_iec104_control(uint32_t ioa);
/* 命令 IOA 是否落在控制段且有控制点映射；成功时写出 tag_id。 */
int iems_iec104_control_tag(uint32_t ioa, edge_tag_id_t *tag_id);
/* 自发上报的按点死区（edge_slave_source_ops_t.deadband 的实现）。 */
double iems_iec104_deadband(void *ctx, uint32_t point_id);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_DRIVER_ADAPTER_H
