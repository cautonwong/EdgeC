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

// --- 行为掩码定义 ---
#define FLAG_SOE 0x00000001

// --- 测点元数据 ---
typedef struct {
    uint32_t id;
    const char *name;
    const char *type;
    const char *unit;
    const char *category;
    double min;  // [修复] 统一为 min/max
    double max;
    bool persist;
    bool upload;
    uint32_t poll_interval_ms; /**< [Phase 4] 自动轮询周期 (0表示不自动轮询) */
} edge_tag_meta_t;

// --- 通用协议映射结构 (Phase 4 增强版) ---
typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;  // [新增]
    float scaler;
    uint8_t type_id;  /**< ASDU TypeID */
    uint32_t flags;   /**< 行为标志位 (如 FLAG_SOE) */
    float dead_band;  /**< 死区阈值 */
    uint8_t group_id; /**< 总召唤分组 */
    uint16_t length;
    uint32_t access;
} iec104_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;  // [新增]
    float scaler;
    uint8_t type_id;
    uint32_t flags;
    float dead_band;
    uint8_t group_id;
    uint16_t length;
    uint32_t access;
} iec101_map_t;

// --- 契约 Bundle 定义 (用于依赖注入) ---
typedef struct {
    const void *const *gi_list;
    uint32_t gi_count;
} iec104_contract_t;

typedef struct {
    const void *const *gi_list;
    uint32_t gi_count;
} iec_contract_bundle_t;

typedef struct {
    uint8_t obis[6];
    uint16_t class_id;
    uint8_t attribute_index;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;  // [新增]
    float scaler;
    uint32_t access;
} dlms_map_t;

typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;  // [新增]
    float scaler;
    uint16_t length;
    uint32_t access;
} modbus_map_t;

// [修复] 补全缺失的协议映射结构
typedef struct {
    uint32_t key;
    edge_tag_id_t tag_id;
    uint16_t dense_idx;  // [新增]
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

// 向后兼容
typedef modbus_map_t tag_map_t;

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

// --- 硬件资源元数据 ---
typedef struct {
    const char *type;
    int id;
    const char *label;
} hw_resource_info_t;

// --- 数据处理管道元数据 (S17: Logic Orchestration) ---
typedef struct {
    const char *name;
    const char *type;
    struct {
        double f1, f2;  /**< 常用浮点参数 (如 alpha, threshold) */
        uint32_t u1;    /**< 常用整型参数 (如 target_tag) */
        const char *s1; /**< 字符串参数 (如 script path) */
    } params;
} edge_stage_def_t;

typedef struct {
    const char *id;
    uint32_t trigger_tag;
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
    // 移除未使用的 layout 等字段，或根据 meta_ui.tmpl 补全
} ui_info_t;

// --- API 路由元数据 ---
typedef struct {
    const char *path;
    edge_tag_id_t tag_id;
    const char *group;
} api_route_t;

#endif
