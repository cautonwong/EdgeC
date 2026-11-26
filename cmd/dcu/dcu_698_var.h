/**
 * @file dcu_698_var.h
 * @brief 698 变量类（Class 1/3/4/6）读取处理器与带类型标签的值
 *
 * 值来自 DataHub 影子；协议量纲换算归 `dcu_driver_adapter`，工程值到标准整数
 * （raw = value × 10^-scaler）在本文件完成。
 */

#ifndef DCU_698_VAR_H
#define DCU_698_VAR_H

#include <stdint.h>

#include "dcu_698_object_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_698_TAG_ARRAY 0x01
#define DCU_698_TAG_STRUCTURE 0x02
#define DCU_698_TAG_BOOLEAN 0x03
#define DCU_698_TAG_DOUBLE_LONG 0x05
#define DCU_698_TAG_DOUBLE_LONG_UNSIGNED 0x06
#define DCU_698_TAG_INTEGER 0x0F
#define DCU_698_TAG_LONG 0x10
#define DCU_698_TAG_UNSIGNED 0x11
#define DCU_698_TAG_LONG_UNSIGNED 0x12
#define DCU_698_TAG_LONG64_UNSIGNED 0x15
#define DCU_698_TAG_ENUM 0x16
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

/** @brief 变量类 OI（0010/0020/2000/2001/2004/200A/200F）返回 true。 */
bool dcu_698_var_is_oi(uint16_t oi);

/**
 * @brief 按标准形状编码变量类的一整个属性（ADR-0016）。
 *
 * 属性 2/4 输出 array，属性 3/5 输出 Scaler_Unit 结构；元素索引选择表槽位。
 * 返回 0 或 `EDGE_SLAVE_SRC_*`；缓冲不足返回 TYPE_MISMATCH，影子缺失返回 NOT_FOUND。
 */
int dcu_698_var_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

/**
 * @brief 按显式 device_id（稳定 slot 身份）编码变量属性；事件 A2/A6 快照使用。
 *
 * OAD 低字节仍只作属性/元素选择，不能当设备身份。其余语义与
 * dcu_698_var_encode_axdr 相同。
 */
int dcu_698_var_encode_axdr_device(uint32_t oad, uint32_t device_id, uint8_t *buf, size_t cap,
                                   size_t *len);

/** @brief 变量类数组用到的全部影子 tag（采集方案从这里派生，避免两处漂移）。 */
size_t dcu_698_var_poll_tags(uint32_t *out, size_t cap);

/** @brief 影子 tag 的北向 metric 名；不在变量点表里返回 NULL。 */
const char *dcu_698_var_metric_name(uint32_t tag);

/** @brief metric 名 → 影子 tag（`dcu_698_var_metric_name` 的反函数）；未知返回 -1。 */
int dcu_698_var_metric_resolve(const char *name, uint32_t *tag);

/**
 * @brief 按 OAD 取带类型标签的变量值。
 *
 * 只服务 Class 6 的属性 2（当前 200F）；Class 1/3/4 的属性 2 是数组，
 * 走 `dcu_698_var_encode_axdr`。影子缺失或 OI/属性不支持返回 -1。
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
