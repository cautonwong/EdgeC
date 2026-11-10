/**
 * @file driver_dlt698.h
 * @brief DLT698 驱动的点位描述（产品侧填，驱动只读）
 */
#ifndef DRIVER_DLT698_H
#define DRIVER_DLT698_H

#include <stdint.h>

/**
 * @brief 写请求的 A-XDR 数据类型（经 `point_desc` 传入）。
 *
 * 只列 codec 里**已有正确编码器**的类型。没有编码器的类型硬写等于猜字节序，
 * 宁可拒绝也不发一个"看起来对"的报文。
 */
typedef enum {
    DLT698_WRITE_LONG_UNSIGNED = 0x12, /**< 2 字节无符号：电压/电流等 */
    DLT698_WRITE_DOUBLE_LONG = 0x05,   /**< 4 字节有符号：电能量等 */
} dlt698_write_type_t;

typedef struct {
    uint8_t type; /**< dlt698_write_type_t */
} dlt698_write_desc_t;

#endif /* DRIVER_DLT698_H */
