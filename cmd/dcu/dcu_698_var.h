/**
 * @file dcu_698_var.h
 * @brief 698 变量类（Class 1/3/4/6）读取处理器与带类型标签的值
 *
 * 值来自 DataHub 影子；量纲不在本文件复制，换算仍归 `dcu_driver_adapter`。
 * 类型标签按 `docs/design/DCU_698_OBJECT_MODEL.md` 5.1。
 */

#ifndef DCU_698_VAR_H
#define DCU_698_VAR_H

#include <stdint.h>

#include "dcu_698_object_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_698_TAG_BOOLEAN 0x03
#define DCU_698_TAG_DOUBLE_LONG_UNSIGNED 0x06
#define DCU_698_TAG_LONG 0x11
#define DCU_698_TAG_LONG64_UNSIGNED 0x15
#define DCU_698_TAG_LONG_UNSIGNED 0x12
#define DCU_698_TAG_UNSIGNED 0x13
#define DCU_698_TAG_FLOAT32 0x17

/** 设计 4.2 的标量便捷结构：供 Class 1 到 6 的处理器内部使用，不是端口契约。 */
typedef struct {
    uint8_t type_tag;
    uint8_t width;
    int64_t raw;
    double value;
    uint64_t ts_ms;
    uint8_t quality;
} dcu_value_t;

/** @brief A-XDR 类型的字节宽度；未知标签返回 0。 */
uint8_t dcu_698_tag_width(uint8_t tag);

/**
 * @brief 按 OAD 取带类型标签的变量值。
 *
 * 支持的首批 OI：0010、0020（Class 1）、2000、2001（Class 3）、2004、200A（Class 4）、
 * 200F（Class 6）。影子缺失、OI 未登记或属性不支持返回 -1。
 */
int dcu_698_var_read_value(uint32_t oad, dcu_value_t *out);

/**
 * @brief 698 从站取数的唯一类型来源：变量类 + 对象表标量属性。
 *
 * 复杂属性（数组/结构）没有标量表示，返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`。
 */
int dcu_698_value_for_oad(uint32_t oad, dcu_value_t *out);

/** @brief 对象表处理器：取 `dcu_value_t` 后按类型标签写回旧式标量槽位。 */
int dcu_698_var_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx, void *out_val);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_VAR_H */
