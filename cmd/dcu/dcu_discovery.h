#ifndef DCU_DISCOVERY_H
#define DCU_DISCOVERY_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

/**
 * @brief 搜表状态（表 A.7 6002 属性 10 的枚举，只有空闲/搜表中两种）。
 *
 * 标准没有"已完成"状态：搜表时长到期即回到空闲，计数保留到方法 128 清空。
 */
typedef enum { DISCOVERY_STATUS_IDLE = 0, DISCOVERY_STATUS_SCANNING = 1 } discovery_status_t;

/**
 * @brief 自动搜表业务管理器 (OI 6002)
 *
 * 产品没有下行搜表驱动（没有广播/地址段扫描实现），因此结果集只能为空；
 * 本结构只维护标准要求的状态与计数，不伪造搜到的 TSA。
 */
typedef struct {
    uint8_t status;         // 当前搜表状态 (discovery_status_t)
    uint32_t duration_min;  // 本次搜表时长（分钟，方法 127 参数）；0 = 不限时间
    uint64_t started_ms;    // 启动时刻，单调毫秒（uv_hrtime/1e6）
    uint16_t count;         // 搜表结果记录数（A3/A6）；无扫描驱动时恒为 0
    uv_mutex_t lock;
} dcu_discovery_t;

void dcu_discovery_init(dcu_discovery_t* disc);

/**
 * @brief 实时启动搜表（表 A.7 6002 方法 127）
 * @param duration_min 搜表时长，单位分钟；0 表示不限时间直至搜表结束
 * @return 0 成功；EDGE_SLAVE_SRC_DENIED 表示已在搜表中，拒绝第二次启动
 */
int dcu_discovery_start(dcu_discovery_t* disc, uint32_t duration_min);

/**
 * @brief 清空搜表结果（表 A.7 6002 方法 128）
 */
void dcu_discovery_clear(dcu_discovery_t* disc);

/**
 * @brief 搜表状态推进（由现有轮询定时器路径 `dcu_node_pump()` 调用）
 * @param now_ms 单调毫秒时刻；时长到期后状态回到空闲
 */
void dcu_discovery_tick(dcu_discovery_t* disc, uint64_t now_ms);

void dcu_discovery_snapshot(dcu_discovery_t* disc, uint8_t* out_status, uint16_t* out_count);

void dcu_discovery_destroy(dcu_discovery_t* disc);

#endif  // DCU_DISCOVERY_H
