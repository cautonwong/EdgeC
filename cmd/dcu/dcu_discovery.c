#include "dcu_discovery.h"

#include <stdio.h>

#include "edge_slave_plane.h"

#define MS_PER_MINUTE 60000ULL

void dcu_discovery_init(dcu_discovery_t* disc) {
    if (!disc) return;

    disc->status = DISCOVERY_STATUS_IDLE;
    disc->duration_min = 0;
    disc->started_ms = 0;
    disc->count = 0;

    uv_mutex_init(&disc->lock);
}

int dcu_discovery_start(dcu_discovery_t* disc, uint32_t duration_min) {
    if (!disc) return EDGE_SLAVE_SRC_TEMPORARY;

    uv_mutex_lock(&disc->lock);
    if (disc->status == DISCOVERY_STATUS_SCANNING) {
        uv_mutex_unlock(&disc->lock);
        return EDGE_SLAVE_SRC_DENIED;
    }

    disc->status = DISCOVERY_STATUS_SCANNING;
    disc->duration_min = duration_min;
    disc->started_ms = uv_hrtime() / 1000000ULL;
    /* 标准 A8 的"每次搜表前清空"选项；产品未暴露 A8，固定按该选项处理。 */
    disc->count = 0;
    uv_mutex_unlock(&disc->lock);

    printf("[DISCOVERY] 搜表计时开始：%u 分钟%s\n", duration_min,
           duration_min == 0 ? "（不限时）" : "");
    return 0;
}

void dcu_discovery_clear(dcu_discovery_t* disc) {
    if (!disc) return;

    uv_mutex_lock(&disc->lock);
    disc->count = 0;
    uv_mutex_unlock(&disc->lock);
}

void dcu_discovery_tick(dcu_discovery_t* disc, uint64_t now_ms) {
    if (!disc) return;

    int completed = 0;
    uint16_t count = 0;
    uv_mutex_lock(&disc->lock);
    if (disc->status == DISCOVERY_STATUS_SCANNING && disc->duration_min != 0) {
        uint64_t duration_ms = (uint64_t)disc->duration_min * MS_PER_MINUTE;
        /* 时长为 0 表示不限时；没有扫描引擎能宣告结束，保持搜表中。 */
        if (now_ms >= disc->started_ms && now_ms - disc->started_ms >= duration_ms) {
            disc->status = DISCOVERY_STATUS_IDLE;
            count = disc->count;
            completed = 1;
        }
    }
    uv_mutex_unlock(&disc->lock);

    if (completed)
        printf("[DISCOVERY] 搜表时长到期，状态回到空闲；结果记录数 %u\n", (unsigned)count);
}

void dcu_discovery_snapshot(dcu_discovery_t* disc, uint8_t* out_status, uint16_t* out_count) {
    if (!disc) return;

    uv_mutex_lock(&disc->lock);
    if (out_status) *out_status = disc->status;
    if (out_count) *out_count = disc->count;
    uv_mutex_unlock(&disc->lock);
}

void dcu_discovery_destroy(dcu_discovery_t* disc) {
    if (!disc) return;
    uv_mutex_destroy(&disc->lock);
}
