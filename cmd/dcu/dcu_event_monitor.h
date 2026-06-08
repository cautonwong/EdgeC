#ifndef DCU_EVENT_MONITOR_H
#define DCU_EVENT_MONITOR_H

#include "stream_engine.h"
#include "dcu_router.h"

// === [Consumer-Defined Interfaces] ===
typedef struct {
    void *ctx;
    int (*archive_event)(void *ctx, const char *addr, uint16_t event_code, uint64_t start_time, uint64_t end_time, float snapshot_val);
    int (*archive_freeze)(void *ctx, const char *addr, uint32_t freeze_oad, uint64_t freeze_time, uint32_t src_oad, double value);
} event_sink_ops_t;

/**
 * @brief 创建失压/过流流式监控判定算子 (stage_event_monitor)
 * @param evt_3000 失压事件对象内存指针 (用于读取实时契约参数)
 * @param evt_3005 过流事件对象内存指针
 */
pipeline_stage_t *stage_event_monitor_create(dcu_event_obj_t *evt_3000, dcu_event_obj_t *evt_3005);

/**
 * @brief 创建事件快照追加与主动上报算子 (stage_event_logger)
 */
pipeline_stage_t *stage_event_logger_create(dcu_event_obj_t *evt_3000, dcu_event_obj_t *evt_3005, datahub_t *hub, void *legacy_storage, event_sink_ops_t sink_ops);

/**
 * @brief 创建日冻结定期快照算子 (stage_periodic_freeze)
 */
pipeline_stage_t *stage_periodic_freeze_create_ex(void *legacy_storage, datahub_t *hub, event_sink_ops_t sink_ops);

#endif // DCU_EVENT_MONITOR_H
