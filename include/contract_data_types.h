#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Zephyr-style IS_ENABLED Macro Machinery
 * Allows for compile-time logic branching in C code.
 */
#define __ARG_PLACEHOLDER_1 0,
#define __take_second_arg(__ignored, val, ...) val
#define __is_defined(x) __is_defined2(x)
#define __is_defined2(x) __take_second_arg(__ARG_PLACEHOLDER_##x 1, 0)

#define IS_ENABLED(config_macro) __is_defined(config_macro)

/**
 * @brief 抽象 TagID 类型
 * 解耦具体的枚举值，Core 逻辑层只认此类型
 */
typedef uint32_t edge_tag_id_t;

/**
 * @brief Tag 元数据结构 (L1 语义层)
 */
typedef struct {
    const char* name;
    const char* unit;
    bool persist;
    bool upload;
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
 * @brief DLMS 协议映射项 (增加编解码元数据)
 */
typedef struct {
    const char* obis;   // OBIS 编码
    edge_tag_id_t id;   // 映射的目标 TagID
    uint16_t ic_id;     // Interface Class ID (如 3: Register)
    uint8_t attr_id;    // Attribute ID (如 2: Value)
    uint8_t data_type;  // DLMS 定义的数据类型编码
    float scaler;       // 缩放系数
} dlms_map_t;
/**
 * @brief IEC104 协议映射项 (L2 物理层)
 */
typedef struct {
    int ioa;
    edge_tag_id_t id;
    int type_id;
} iec104_map_t;

/**
 * @brief CAN 总线信号映射项 (L2 物理层)
 */
typedef struct {
    uint32_t can_id;
    edge_tag_id_t id;
    int start;
    int len;
    float factor;
    float offset;
} can_map_t;

// --- 外部契约接口声明 (由生成的 contracts.c 实现) ---
extern const tag_meta_t g_tag_meta_table[];
const tag_meta_t* edge_get_meta(edge_tag_id_t id);

edge_tag_id_t edge_lookup_modbus(uint16_t reg, float* out_scale);
edge_tag_id_t edge_lookup_dlms(const char* obis, float* out_scale);
edge_tag_id_t edge_lookup_dlt645(const char* di, float* out_scale);
edge_tag_id_t edge_lookup_dlt698(const char* oaid, float* out_scale);
edge_tag_id_t edge_lookup_iec104(int ioa);

#endif  // CONTRACT_DATA_TYPES_H