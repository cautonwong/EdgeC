#ifndef DCU_TASK_STORE_H
#define DCU_TASK_STORE_H

#include <stdint.h>

#include "dcu_poller.h"
#include "dcu_storage.h"

/* 落库的任务配置子集：只存调度器真正执行的字段，未实现的字段在 apply 时拒绝。 */
typedef struct {
    uint8_t task_id;
    uint8_t interval_unit;
    uint16_t interval_value;
    uint8_t scheme_type;
    uint8_t scheme_id;
    uint8_t status;
    uint8_t period_type;
} dcu_task_cfg_t;

/** @brief 建表 + 首次启动播种默认任务（ID 1、5 s、方案 1），只播种一次。 */
int dcu_task_store_init(dcu_storage_t *storage);

/** @brief 把库里的任务装进 poller；坏行跳过并告警，不清库。 */
int dcu_task_store_load(dcu_storage_t *storage, dcu_poller_t *poller);

/** @brief 6012 方法 127/128/129/130 的事务化落地；失败回滚 DB 与 RAM。 */
int dcu_task_store_apply(dcu_storage_t *storage, dcu_poller_t *poller, const dcu_task_op_t *op);

#endif /* DCU_TASK_STORE_H */
