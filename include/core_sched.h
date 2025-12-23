#ifndef CORE_SCHED_H
#define CORE_SCHED_H

#include <time.h>
#include <stddef.h>

// Forward declare uv_loop_t to avoid including the full uv.h here
typedef struct uv_loop_s uv_loop_t;

typedef void (*sched_cb)(void *arg);

// Cron 任务句柄
typedef struct cron_task cron_task_t;

/**
 * @brief 初始化调度器
 *
 * @param loop libuv 事件循环
 * @param max_tasks 预估的最大 Cron 任务数
 * @return 0 on success, -1 on error.
 */
int core_sched_init(uv_loop_t *loop, size_t max_tasks);

/**
 * @brief 添加一个 Cron 任务
 *
 * @param cron_str 标准 Cron 表达式 (e.g., "0 * * * * *")
 * @param cb 回调函数
 * @param arg 用户参数
 * @return 任务句柄，失败返回 NULL
 */
cron_task_t* core_sched_add_cron(const char *cron_str, sched_cb cb, void *arg);

/**
 * @brief 销毁调度器
 */
void core_sched_destroy(void);

#endif // CORE_SCHED_H
