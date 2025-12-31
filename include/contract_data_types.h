#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 基础宏：用于编译时的功能裁剪
 */
#define IS_ENABLED(x) ((x) == 1)

/**
 * @brief 抽象 TagID 类型
 * 解耦具体的枚举值，Core 逻辑层只认此类型
 */
typedef uint32_t edge_tag_id_t;

/**
 * @brief Tag 元数据结构 (L1 语义层)
 */
typedef struct {
    const char* name;  // 语义名称
    const char* unit;  // 单位
    bool persist;      // 持久化
    bool upload;       // 上送
} tag_meta_t;

/**
 * @brief Modbus 协议映射项 (L2 物理层)
 */
typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scale;
} modbus_map_t;

/**
 * @brief 字符串类协议通用映射项 (DLMS/645/698)
 */
typedef struct {
    const char* obis;  // 或 DI / OAID
    edge_tag_id_t id;
    uint8_t ic;       // Class ID
    uint8_t attr_id;  // Attribute ID
    uint8_t type;     // 数据类型
    float scaler;
} dlms_map_t;

/**
 * @brief IEC104 协议映射项
 */
typedef struct {
    int ioa;
    edge_tag_id_t id;
    int type_id;
} iec104_map_t;

/**
 * @brief CAN 总线信号映射项
 */
typedef struct {
    uint32_t can_id;
    edge_tag_id_t id;
    int start;
    int len;
    float factor;
    float offset;
} can_map_t;

#endif  // CONTRACT_DATA_TYPES_H