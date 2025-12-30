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
 * @brief DLMS 协议映射项 (L2 物理层)
 */
typedef struct {
    const char* obis;
    edge_tag_id_t id;
    float scaler;
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