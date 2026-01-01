#ifndef CONTRACT_DATA_TYPES_H
#define CONTRACT_DATA_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 基础宏与类型定义 (Frozen Layer 0)
 */
#define IS_ENABLED(x) ((x) == 1)
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

// --- 协议映射结构 (统一定义，解决编译依赖) ---

typedef struct {
    uint16_t addr;
    edge_tag_id_t id;
    float scale;
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
    edge_tag_id_t id;
    float scale;
} dlt645_map_t;

typedef struct {
    const char* oad;
    edge_tag_id_t id;
    float scale;
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
