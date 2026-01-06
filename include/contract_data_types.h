#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stddef.h>
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
typedef uint32_t edge_param_id_t;  // [新增] 参数 ID 基础类型

// --- 访问权限定义 ---
#define ACCESS_READ 0x00000001
#define ACCESS_WRITE 0x00000002
#define ACCESS_NOTIFY 0x00000004

// --- 测点元数据 ---
typedef struct {
    const char *name;
    const char *unit;
    const char *category;
    const char *ui_group;
    float min_val;
    float max_val;
    bool persist;
    bool upload;
} tag_meta_t;

// --- 协议绑定结构 ---

typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} modbus_map_t;

typedef struct {
    uint16_t class_id;
    const char *obis;
    uint8_t attr_id;
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
    const char *di;
    uint32_t di_num;
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt645_map_t;

typedef struct {
    const char *oad;
    uint32_t oad_num;
    edge_tag_id_t id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt698_map_t;

typedef struct {
    uint32_t can_id;
    edge_tag_id_t id;
    int start_bit;
    int len;
    float factor;
    float offset;
    uint32_t access;
} can_map_t;

// --- 运行期参数元数据 (整定参数 V4.5) ---

typedef enum { PARAM_TYPE_INT = 1, PARAM_TYPE_FLOAT = 2, PARAM_TYPE_STRING = 3 } edge_param_type_t;

typedef union {
    int64_t i;
    double f;
    const char *s;
} edge_param_value_t;

typedef struct {
    const char *name;

    edge_param_type_t type;

    edge_param_value_t def_val;

    edge_param_value_t min_val;

    edge_param_value_t max_val;

    bool is_reboot_required;

} edge_param_info_t;

// --- 硬件 DTS 元数据 (Hardware V4.5) ---

typedef struct {
    const char *label;

    const char *type;

    int id;

    const char *bus_ref;

    const char *attributes;  // JSON 格式或 Key-Value 字符串

} hw_resource_info_t;

// --- 流引擎管线元数据 (Pipeline V4.5) ---

typedef struct {
    const char *id;

    uint32_t trigger_tag;

    const char *description;

    const char *layout;  // 描述 Stage 序列的 JSON 字符串

} pipeline_info_t;

// --- 策略与北向策略 (Policy V4.5) ---

typedef struct {
    const char *name;

    const char *type;

    const char *content;  // 策略详细参数 (JSON)

} policy_info_t;

// --- UI 展现元数据 (UI V4.5) ---

typedef struct {
    const char *layout;

    const char *dashboards;  // 看板配置 (JSON)

} ui_info_t;

#endif
