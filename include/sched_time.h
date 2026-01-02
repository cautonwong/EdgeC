#ifndef SCHED_TIME_H
#define SCHED_TIME_H

#include <stdint.h>
#include <time.h>
#include <uv.h>

/**
 * @file sched_time.h
 * @brief 调度平面公共头文件 (支持 DCU 周期对齐与 S10 守时)
 */

typedef uint64_t wall_time_t;
typedef uint64_t mono_time_t;

// [S13/S14] 任务类型定义
typedef enum {
    CAL_TASK_MINUTE_FREEZE = 1,
    CAL_TASK_HOUR_FREEZE = 2,
    CAL_TASK_DAY_FREEZE = 3,
    CAL_TASK_PERIODIC_READ = 4
} calendar_task_type_t;

/**
 * @brief 挂钟调度任务结构
 */
typedef struct calendar_task_s {
    uv_timer_t timer;
    calendar_task_type_t type;
    uint32_t period_sec;
    uint32_t offset_sec;
    uint8_t align_min;  // 用于 15 分钟对齐等

    wall_time_t target_wall;

    void (*on_fire)(struct calendar_task_s *task);
    void *user_data;

    uv_loop_t *loop;  // 物理绑定的 Loop
} calendar_task_t;

// 物理接口声明
wall_time_t sched_wall_now(void);
mono_time_t sched_mono_now(void);
void calendar_schedule(calendar_task_t *task);
void calendar_cancel(calendar_task_t *task);
wall_time_t calc_next_boundary(calendar_task_t *task, wall_time_t now);

/**
 * @brief 计算相对于 00:00:00 对齐的下一次触发时间 (ms)
 */
static inline uint64_t sched_calc_next_align_ms(uint64_t now_ms, uint32_t period_sec,
                                                uint32_t offset_sec) {
    if (period_sec == 0) return now_ms;
    uint64_t now_sec = now_ms / 1000;
    time_t sec = (time_t)now_sec;
    struct tm t;
    gmtime_r(&sec, &t);
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    uint64_t day_start_sec = (uint64_t)timegm(&t);
    uint64_t elapsed = now_sec - day_start_sec;
    uint64_t next_intervals = (elapsed - offset_sec) / period_sec + 1;
    return (day_start_sec + offset_sec + (next_intervals * period_sec)) * 1000;
}

#endif