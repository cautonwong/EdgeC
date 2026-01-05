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
    const char* name;      // 8
    const char* unit;      // 8
    const char* category;  // 8
    const char* ui_group;  // 8
    float min_val;         // 4
    float max_val;         // 4
    bool persist;          // 1
    bool upload;           // 1
    uint8_t reserved[2];   // 2 (Explicit padding for 4-byte alignment)
} tag_meta_t;

// --- 协议绑定结构 (物理层面 - V4.5 升维版) ---

typedef struct {
    edge_tag_id_t id;   // 4
    float scaler;       // 4
    uint32_t access;    // 4
    uint16_t addr;      // 2
    uint16_t length;    // 2
} modbus_map_t;

typedef struct {
    const char* obis;   // 8
    uint64_t raw_key;   // 8 (Pre-calculated uint64 OBIS)
    edge_tag_id_t id;   // 4
    float scaler;       // 4
    uint32_t access;    // 4
    uint16_t class_id;  // 2
    uint16_t length;    // 2
    uint8_t attr_id;    // 1
    uint8_t reserved[3];// 3
} dlms_map_t;

typedef struct {
    edge_tag_id_t id;   // 4
    int ioa;            // 4
    int type_id;        // 4
    uint32_t access;    // 4
    uint16_t length;    // 2
    uint16_t reserved;  // 2
} iec104_map_t;

typedef struct {
    const char* di;     // 8
    edge_tag_id_t id;   // 4
    uint32_t di_num;    // 4
    float scaler;       // 4
    uint32_t access;    // 4
    uint16_t length;    // 2
    uint16_t reserved;  // 2
} dlt645_map_t;

typedef struct {
    const char* oad;    // 8
    edge_tag_id_t id;   // 4
    uint32_t oad_num;   // 4
    float scaler;       // 4
    uint32_t access;    // 4
    uint16_t length;    // 2
    uint16_t reserved;  // 2
} dlt698_map_t;

// Static assertions to ensure memory layout sovereignty (C11)
#if !defined(__cplusplus) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(modbus_map_t) == 16, "modbus_map_t size mismatch (padding detected)");
_Static_assert(sizeof(iec104_map_t) == 20, "iec104_map_t size mismatch");
_Static_assert(sizeof(dlt645_map_t) == 32, "dlt645_map_t size mismatch");
#endif

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
