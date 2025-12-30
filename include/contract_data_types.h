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
    const char* name;  // 语义名称，如 "VOLTAGE_PHASE_A"
    const char* unit;  // 单位，如 "V"
    bool persist;      // 是否允许持久化到 SQLite
    bool upload;       // 是否允许上送云端
} tag_meta_t;

/**
 * @brief Modbus 协议映射项 (L2 物理层)
 */
typedef struct {
    uint16_t addr;     // 寄存器起始地址
    edge_tag_id_t id;  // 映射的目标 TagID
    float scale;       // 缩放系数 (读取值 * scale)
} modbus_map_t;

/**
 * @brief DLMS 协议映射项 (L2 物理层)
 */
typedef struct {
    const char* obis;  // OBIS 编码，如 "1.0.32.7.0.255"
    edge_tag_id_t id;  // 映射的目标 TagID
    float scaler;      // 缩放系数
} dlms_map_t;

/**
 * @brief IEC104 协议映射项 (L2 物理层)
 */
typedef struct {
    int ioa;           // 信息对象地址
    edge_tag_id_t id;  // 映射的目标 TagID
    int type_id;       // ASDU 类型 ID
} iec104_map_t;

/**
 * @brief CAN 总线信号映射项 (L2 物理层)
 */
typedef struct {
    uint32_t can_id;   // 帧 ID
    edge_tag_id_t id;  // 映射的目标 TagID
    int start;         // 起始位
    int len;           // 长度
    float factor;      // 因子
    float offset;      // 偏移量
} can_map_t;

#endif  // CONTRACT_DATA_TYPES_H
