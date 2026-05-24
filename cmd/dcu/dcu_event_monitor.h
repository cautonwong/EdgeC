#ifndef DCU_EVENT_MONITOR_H
#define DCU_EVENT_MONITOR_H

#include "stream_engine.h"
#include "dcu_router.h"

/**
 * @brief 创建失压/过流流式监控判定算子 (stage_event_monitor)
 * @param evt_3000 失压事件对象内存指针 (用于读取实时契约参数)
 * @param evt_3005 过流事件对象内存指针
 */
pipeline_stage_t *stage_event_monitor_create(dcu_event_obj_t *evt_3000, dcu_event_obj_t *evt_3005);

/**
 * @brief 创建事件快照追加与主动上报算子 (stage_event_logger)
 * @param evt_3000 失压事件对象内存指针 (用于将快照塞入事件记录表)
 * @param evt_3005 过流事件对象内存指针
 * @param hub DataHub 总线底座 (用于抓取其它 OAD 的实时快照值)
 */
pipeline_stage_t *stage_event_logger_create(dcu_event_obj_t *evt_3000, dcu_event_obj_t *evt_3005, datahub_t *hub);

/**
 * @brief 创建日冻结定期快照算子 (stage_periodic_freeze)
 * @param storage SQLite 集中器存储服务句柄
 * @param hub DataHub 总线底座
 */
pipeline_stage_t *stage_periodic_freeze_create_ex(dcu_storage_t *storage, datahub_t *hub);

#endif // DCU_EVENT_MONITOR_H
