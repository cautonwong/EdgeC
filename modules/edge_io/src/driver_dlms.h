/**
 * @file driver_dlms.h
 * @brief DLMS 驱动的点位描述（产品侧填，驱动只读）
 */
#ifndef DRIVER_DLMS_H
#define DRIVER_DLMS_H

#include <stdint.h>

/**
 * @brief 写请求的 Data 类型（经 `point_desc` 传入）。
 *
 * 只列 codec 能正确编码的类型。类型是**点位模型的知识**（同一个 OBIS 在不同表上
 * 可能是 long-unsigned 也可能是 double-long-unsigned），驱动不猜。
 *
 * 填在 `dlms_point_t.data_type` 里（点位描述的一部分），读请求忽略。
 */
typedef enum {
    DLMS_WRITE_LONG_UNSIGNED = 0x12,        /**< 2 字节无符号 */
    DLMS_WRITE_DOUBLE_LONG_UNSIGNED = 0x06, /**< 4 字节无符号 */
} dlms_write_type_t;

/**
 * @brief 点位描述：调用方按这个结构填，作为 `point_desc` 传进来。
 *
 * DLMS 没有能塞进 32 位 native_id 的标识（OBIS 就 48 位），所以点位信息全部走描述。
 */
typedef struct {
    uint8_t obis[6];
    uint16_t class_id;
    uint8_t attribute_index;
    uint8_t data_type; /**< 写请求的 Data 类型；读请求忽略 */
} dlms_point_t;

#endif /* DRIVER_DLMS_H */
