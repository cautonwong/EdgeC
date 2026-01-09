/**
 * @file iec_cp56time.h
 * @brief IEC 60870-5-101/104 CP56Time2a (7字节) 时标处理
 */

#ifndef EDGE_IEC_CP56TIME_H
#define EDGE_IEC_CP56TIME_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CP56Time2a 结构定义 (7 字节)
 */
#pragma pack(push, 1)
typedef struct {
    uint16_t msec;     /**< 毫秒: 0..59999 */
    uint8_t min : 6;   /**< 分钟: 0..59 */
    uint8_t res1 : 1;  /**< 预留 */
    uint8_t iv : 1;    /**< 无效标志 (Invalid) */
    uint8_t hour : 5;  /**< 小时: 0..23 */
    uint8_t res2 : 2;  /**< 预留 */
    uint8_t su : 1;    /**< 夏令时标志 (Summer Time) */
    uint8_t mday : 5;  /**< 每月几号: 1..31 */
    uint8_t wday : 3;  /**< 星期几: 1..7 (1=周一) */
    uint8_t month : 4; /**< 月份: 1..12 */
    uint8_t res3 : 4;  /**< 预留 */
    uint8_t year : 7;  /**< 年份: 0..99 (从 2000 年起算) */
    uint8_t res4 : 1;  /**< 预留 */
} iec_cp56time_t;
#pragma pack(pop)

/**
 * @brief 获取当前系统时间的 CP56Time2a 表示
 */
void iec_cp56time_now(iec_cp56time_t *out);

/**
 * @brief 将 CP56Time2a 转换为 Unix 毫秒时间戳
 */
uint64_t iec_cp56time_to_ms(const iec_cp56time_t *ts);

/**
 * @brief 从 Unix 毫秒时间戳构造 CP56Time2a
 */
void iec_cp56time_from_ms(iec_cp56time_t *out, uint64_t ms);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_IEC_CP56TIME_H
