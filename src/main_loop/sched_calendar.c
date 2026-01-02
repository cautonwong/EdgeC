#include "sched_time.h"

/**
 * @file sched_calendar.c
 * @brief 挂钟对齐计算 (兼容性实现)
 */

wall_time_t calc_next_boundary(calendar_task_t *task, wall_time_t now) {
    if (!task) return now;

    uint32_t period = 0;
    switch (task->type) {
        case CAL_TASK_MINUTE_FREEZE:
            period = 60;
            break;
        case CAL_TASK_HOUR_FREEZE:
            period = 3600;
            break;
        case CAL_TASK_DAY_FREEZE:
            period = 86400;
            break;
        case CAL_TASK_PERIODIC_READ:
            period = task->align_min * 60;  // 假设 align_min 是分钟
            break;
        default:
            return now + 1000;
    }

    return sched_calc_next_align_ms(now, period, task->offset_sec);
}