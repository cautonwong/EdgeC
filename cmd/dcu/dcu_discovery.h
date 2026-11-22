#ifndef DCU_DISCOVERY_H
#define DCU_DISCOVERY_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

/** 表 A.7 6002 属性 10：搜表状态只有空闲/搜表中两种。 */
typedef enum { DISCOVERY_STATUS_IDLE = 0, DISCOVERY_STATUS_SCANNING = 1 } discovery_status_t;

/**
 * @brief OI 6002 的状态与结果计数
 *
 * 产品没有下行搜表驱动：方法 127 回 DAR 2，状态恒为空闲，计数恒为 0。
 * 结果集与计数只在 RAM；不伪造搜到的 TSA。
 */
typedef struct {
    discovery_status_t status;
    uint16_t count;  // A3/A6：搜表结果记录数
    uv_mutex_t lock;
} dcu_discovery_t;

void dcu_discovery_init(dcu_discovery_t* disc);

/** @brief 清空搜表结果（表 A.7 6002 方法 128）。 */
void dcu_discovery_clear(dcu_discovery_t* disc);

void dcu_discovery_snapshot(dcu_discovery_t* disc, uint8_t* out_status, uint16_t* out_count);

void dcu_discovery_destroy(dcu_discovery_t* disc);

#endif  // DCU_DISCOVERY_H
