#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Zephyr-style IS_ENABLED Macro Machinery
 * Allows for compile-time logic branching in C code.
 */
#define __take_second_arg(__ignored, val, ...) val
#define __is_defined(x) __is_defined_expand(x)
#define __is_defined_expand(x) __is_defined2(__ARG_PLACEHOLDER_##x)
#define __is_defined2(arg) __take_second_arg(arg 1, 0)
#define __ARG_PLACEHOLDER_1 0,

// [补强] 自动补全后缀，提升开发体验
#define IS_ENABLED(tag) __is_defined(tag##_ENABLED)
typedef uint32_t edge_tag_id_t;

// --- 元数据结构 ---
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

// --- 协议映射结构 (统一步骤：使用 scaler 命名) ---

typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scaler;
} modbus_map_t;

typedef struct {
    const char* obis;
    edge_tag_id_t id;
    float scaler;
} dlms_map_t;

typedef struct {
    int ioa;
    edge_tag_id_t id;
    int type_id;
} iec104_map_t;

typedef struct {
    const char* di;
    uint32_t di_num;
    edge_tag_id_t id;
    float scaler;
} dlt645_map_t;

typedef struct {
    const char* oad;
    uint32_t oad_num;
    edge_tag_id_t id;
    float scaler;
} dlt698_map_t;

typedef struct {
    uint32_t can_id;
    edge_tag_id_t id;
    int start;
    int len;
    float factor;
    float offset;
} can_map_t;

#endif