/**
 * @file iems_poll_scheduler.h
 * @brief IEMS 周期采集调度器（从任务库加载点位，按周期经仲裁器取数）
 */

#ifndef IEMS_POLL_SCHEDULER_H
#define IEMS_POLL_SCHEDULER_H

#include <stdbool.h>
#include <uv.h>

#include "iems_node.h"
#include "queue.h"
#include "sched_time.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uv_loop_t *loop;
    iems_node_t *node;
    sqlite_svc_t *db;
    struct uv__queue tasks;
    uv_timer_t load_timer;
    bool load_timer_open;
    bool assets_loaded;
} iems_poll_scheduler_t;

/**
 * @brief 初始化调度器（只记录依赖，不启动定时器）。
 */
void iems_poll_scheduler_init(iems_poll_scheduler_t *s, uv_loop_t *loop, iems_node_t *node,
                              sqlite_svc_t *db);

/**
 * @brief 注册任务加载定时器，持续重试直到任务库里出现任务。
 */
void iems_poll_scheduler_start(iems_poll_scheduler_t *s);

/**
 * @brief 停止加载定时器并释放所有采集任务。
 */
void iems_poll_scheduler_stop(iems_poll_scheduler_t *s);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_POLL_SCHEDULER_H
