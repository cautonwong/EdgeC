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
typedef uint32_t edge_param_id_t;

// --- 访问权限定义 ---
#define ACCESS_READ 0x00000001
#define ACCESS_WRITE 0x00000002
#define ACCESS_NOTIFY 0x00000004

// --- 测点元数据 (对齐 V4.5 业务逻辑) ---
typedef struct {
    uint32_t id;
    const char *name;
    const char *type;
    const char *unit;
    const char *category;
    float min_val;
    float max_val;
    bool persist;
    bool upload;
} edge_tag_meta_t;

// --- 通用协议映射结构 ---
typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    float scaler;
    uint16_t length;
    uint32_t access;
} modbus_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt645_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt698_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    float scaler;
    uint16_t length;
    uint32_t access;
} iec104_map_t;

typedef struct {
    uint8_t obis[6];
    uint16_t class_id;
    uint8_t attribute_index;
    edge_tag_id_t tag_id;
    float scaler;
    uint32_t access;
    uint8_t expected_type;  // 对齐 dlms_data_type_t
} dlms_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    float scaler;
    uint16_t length;
    uint32_t access;
} tag_map_t;

// --- 运行期参数元数据 ---
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

// --- 硬件 DTS 元数据 ---
typedef struct {
    const char *label;
    const char *type;
    int id;
    const char *bus_ref;
    const char *attributes;
} hw_resource_info_t;

// --- 流引擎管线元数据 ---
typedef struct {
    const char *id;
    uint32_t trigger_tag;
    const char *config_json;
    int stage_count;
} edge_pipeline_def_t;

// --- 策略与北向策略 ---
typedef struct {
    const char *name;
    const char *cron;
    const char *script;
} edge_policy_def_t;

// --- UI 展现与 API 网关元数据 ---
typedef struct {
    const char *layout;
    const char *dashboards_json;
} ui_info_t;

typedef struct {
    const char *path;
    edge_tag_id_t tag_id;
    const char *category;
} api_route_t;

const edge_param_info_t *edge_get_param_meta(edge_param_id_t id);
uint32_t edge_param_to_tag_id(int param_idx);

#endif
