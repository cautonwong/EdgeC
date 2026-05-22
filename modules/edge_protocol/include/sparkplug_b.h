#ifndef SPARKPLUG_B_H
#define SPARKPLUG_B_H

#include <stdint.h>
#include "net_buf.h"

/**
 * @brief Sparkplug B 2.2 数据类型定义
 */
typedef enum {
    SPB_TYPE_UNKNOWN = 0,
    SPB_TYPE_INT8 = 1,
    SPB_TYPE_INT16 = 2,
    SPB_TYPE_INT32 = 3,
    SPB_TYPE_INT64 = 4,
    SPB_TYPE_UINT8 = 5,
    SPB_TYPE_UINT16 = 6,
    SPB_TYPE_UINT32 = 7,
    SPB_TYPE_UINT64 = 8,
    SPB_TYPE_FLOAT = 9,
    SPB_TYPE_DOUBLE = 10,
    SPB_TYPE_BOOLEAN = 11,
    SPB_TYPE_STRING = 12,
    SPB_TYPE_DATETIME = 13,
    SPB_TYPE_TEXT = 14,
    SPB_TYPE_UUID = 15,
    SPB_TYPE_DATASET = 16,
    SPB_TYPE_BYTES = 17,
    SPB_TYPE_FILE = 18,
    SPB_TYPE_TEMPLATE = 19,
    SPB_TYPE_PROPERTYSET = 20,
    SPB_TYPE_PROPERTYSETLIST = 21
} spb_datatype_t;

// Sparkplug B Protobuf Field Numbers (Payload)
#define SPB_FIELD_TIMESTAMP 1
#define SPB_FIELD_METRICS   2
#define SPB_FIELD_SEQ       3
#define SPB_FIELD_UUID      4
#define SPB_FIELD_BODY      5

// Protobuf Field Numbers (Metric)
#define SPB_METRIC_NAME      1
#define SPB_METRIC_ALIAS     2
#define SPB_METRIC_TIMESTAMP 3
#define SPB_METRIC_DATATYPE  4
#define SPB_METRIC_IS_HIST   5
#define SPB_METRIC_IS_TRANS  6
#define SPB_METRIC_IS_NULL   7
#define SPB_METRIC_FLOAT_VAL 10
#define SPB_METRIC_DOUBLE_VAL 11
#define SPB_METRIC_INT_VAL   12
#define SPB_METRIC_LONG_VAL  13
#define SPB_METRIC_UINT_VAL  14
#define SPB_METRIC_ULONG_VAL 15
#define SPB_METRIC_BOOL_VAL  16
#define SPB_METRIC_STRING_VAL 17

/**
 * @brief 编码 Protobuf Varint
 */
static inline void spb_encode_varint(struct net_buf_simple *b, uint64_t val) {
    while (val >= 0x80) {
        net_buf_simple_add_u8(b, (uint8_t)(val | 0x80));
        val >>= 7;
    }
    net_buf_simple_add_u8(b, (uint8_t)val);
}

/**
 * @brief 编码 Protobuf Tag (Field Number + Wire Type)
 */
static inline void spb_encode_tag(struct net_buf_simple *b, uint32_t field_num, uint8_t wire_type) {
    spb_encode_varint(b, (field_num << 3) | wire_type);
}

/**
 * @brief 编码 String (Wire Type 2)
 */
static inline void spb_encode_string(struct net_buf_simple *b, uint32_t field_num, const char *str) {
    if (!str) return;
    size_t len = strlen(str);
    spb_encode_tag(b, field_num, 2);
    spb_encode_varint(b, len);
    net_buf_simple_add_mem(b, str, len);
}

#endif
