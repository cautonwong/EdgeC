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

// --- 测点元数据 ---
typedef struct {
    uint32_t id;
    const char *name;
    const char *type;
    const char *unit;
    const char *category;
    double min;
    double max;
    bool persist;
    bool upload;
    uint32_t poll_interval_ms;
} edge_tag_meta_t;

// --- 物理通道配置 ---
typedef struct {
    int id;
    const char *label;
    const char *category;
    const char *type;
    const char *simulation_path;
    const char *production_path;
} edge_connectivity_t;

// --- 硬件资源 ---
typedef struct {
    const char *type;
    int id;
    const char *label;
} hw_resource_info_t;

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
    const char *unit;
    double min;
    double max;
    const char *desc;
    bool reboot_required;
    uint32_t id;
} edge_param_info_t;

// --- 数据处理管道元数据 ---
typedef struct {
    const char *name;
    const char *type;
    struct {
        double f1, f2, f3;
        uint32_t u1;
        const char *s1;
    } params;
} edge_stage_def_t;

typedef struct {
    const char *id;
    uint32_t trigger_tag;
    const uint32_t *extra_triggers;
    uint32_t extra_trigger_count;
    const edge_stage_def_t *stages;
    uint32_t stage_count;
} edge_pipeline_def_t;

// --- 决策策略元数据 ---
typedef struct {
    const char *name;
    const char *type;
    struct {
        uint32_t interval;
        const uint32_t *tags;
        uint32_t tag_count;
    } params;
} edge_policy_def_t;

// --- UI 配置元数据 ---
typedef struct {
    const char *theme;
} ui_info_t;

// --- 流处理引擎基础类型前置声明 ---
struct pipeline_stage_s;
struct pipeline_loader_ctx_s;
typedef struct pipeline_stage_s pipeline_stage_t;
typedef struct pipeline_loader_ctx_s pipeline_loader_ctx_t;

// --- 算子工厂注册表基础类型 ---
typedef pipeline_stage_t *(*stage_factory_fn)(const edge_stage_def_t *def,
                                              pipeline_loader_ctx_t *ctx);

typedef struct {
    const char *type;
    stage_factory_fn factory;
} edge_stage_factory_entry_t;

// --- API 路由表项 ---
typedef struct {
    const char *path;
    edge_tag_id_t tag_id;
    const char *category;
} api_route_t;

// --- 驱动契约注入包 ---
typedef struct {
    const void *(*lookup_by_key)(uint32_t key);
    const void *(*lookup_by_tag)(uint32_t tag_id);
    const void *const *meta_list;
    uint32_t meta_list_len;
} driver_contract_injection_t;

// --- 模型注册项 ---
typedef struct {
    const char *model_name;
    const driver_contract_injection_t *injection;
} edge_model_map_t;

// --- 协议映射结构 ---
typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;
    float scaler;
    uint16_t length;
    uint32_t access;
} modbus_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;
    float scaler;
    uint16_t length;
    uint32_t access;
} dlt645_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;
    float scaler;
    uint8_t type_id;
    uint32_t flags;
    float dead_band;
    uint8_t group_id;
    uint16_t length;
    uint32_t access;
} iec104_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;
    float scaler;
    uint8_t type_id;
    uint32_t flags;
    float dead_band;
    uint8_t group_id;
    uint16_t length;
    uint32_t access;
} iec101_map_t;

typedef struct {
    uint8_t obis[6];
    uint16_t class_id;
    uint8_t attribute_index;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;
    float scaler;
    uint32_t access;
} dlms_map_t;

typedef modbus_map_t tag_map_t;

#endif
