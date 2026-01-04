#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Zephyr-style IS_ENABLED Macro Machinery
 */
#define __take_second_arg(__ignored, val, ...) val
#define __is_defined(x) __is_defined_expand(x)
#define __is_defined_expand(x) __is_defined2(__ARG_PLACEHOLDER_##x)
#define __is_defined2(arg) __take_second_arg(arg 1, 0)
#define __ARG_PLACEHOLDER_1 0,

#define IS_ENABLED(tag) __is_defined(tag##_ENABLED)
typedef uint32_t edge_tag_id_t;

// --- 访问权限定义 (V4.5 标准) ---
#define ACCESS_READ 0x00000001
#define ACCESS_WRITE 0x00000002
#define ACCESS_NOTIFY 0x00000004

// --- 测点元数据 (逻辑层面) ---
typedef struct {
    const char* name;      // "VOLTAGE_PHASE_A"
    const char* unit;      // "V"
    const char* category;  // "Metrology", "Storage", etc.
    const char* ui_group;  // UI 分组名称
    float min_val;         // 量程下限
    float max_val;         // 量程上限
    bool persist;          // 是否存库
    bool upload;           // 是否上送
} tag_meta_t;

// --- 协议绑定结构 (物理层面 - V4.5 升维版) ---

typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scaler;
    uint16_t length; /**< 数据字节数 */
    uint32_t access; /**< 读写权限掩码 */
} modbus_map_t;

typedef struct {
    uint16_t class_id; /**< [新增] DLMS Class ID */
    const char* obis;  /**< OBIS code string */
    uint8_t attr_id;   /**< [新增] Attribute ID */
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlms_map_t;

typedef struct {
    int ioa;
    edge_tag_id_t id;
    int type_id;
    uint16_t length;
    uint32_t access;
} iec104_map_t;

typedef struct {
    const char* di;
    uint32_t di_num;
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt645_map_t;

typedef struct {
    const char* oad;
    uint32_t oad_num;
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt698_map_t;

typedef struct {
    uint32_t can_id;
    edge_tag_id_t id;
    int start;
    int len;
    float factor;
    float offset;
    uint32_t access;
} can_map_t;

#endif
