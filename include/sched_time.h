#ifndef SCHED_TIME_H
#define SCHED_TIME_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t wall_time_t;  // ms since epoch
typedef int64_t mono_time_t;  // ms since boot

typedef enum {
    CAL_TASK_MINUTE_FREEZE,
    CAL_TASK_HOUR_FREEZE,
    CAL_TASK_PERIODIC_READ,  // 15/30/60 min alignment
    CAL_TASK_DAY_FREEZE,
    CAL_TASK_MONTH_FREEZE,
    CAL_TASK_PERIODIC_MATH  // 新增：完全借鉴 libev 的数学周期任务
} calendar_task_type_t;

typedef enum {
    CAL_STATE_IDLE,
    CAL_STATE_SCHEDULED,
    CAL_STATE_FIRED,
    CAL_STATE_DONE
} calendar_state_t;

// Persistent state to survive reboots/crashes
typedef struct {
    calendar_task_type_t type;
    wall_time_t last_fired_wall;
} calendar_persist_t;

// Forward declaration
struct calendar_task;

typedef struct calendar_task {
    calendar_task_type_t type;
    calendar_state_t state;

    wall_time_t target_wall;
    mono_time_t target_mono;

    uv_timer_t timer;
    uv_loop_t *loop;

    uint32_t align_min;  // 对于 PERIODIC 任务：对齐分钟数

    // 对于 DAY/MONTH 任务的精确锚点
    uint32_t day;     // 每月第几天 (1-31)
    uint32_t hour;    // 小时 (0-23)
    uint32_t minute;  // 分钟 (0-59)
    uint32_t second;  // 秒 (0-59)

    // 对于 PERIODIC_MATH 任务 (libev 风格)
    uint32_t interval_sec;    // 周期（秒）
    wall_time_t offset_wall;  // 基准偏移时刻 (ms)

    calendar_persist_t persist;

    void (*on_fire)(struct calendar_task *task);
} calendar_task_t;

// Time Source API (Mockable)
wall_time_t sched_wall_now(void);
mono_time_t sched_mono_now(void);

// Global Scheduler Init (Starts Jump Monitoring)
void scheduler_init(uv_loop_t *loop);

// Calendar Logic API
wall_time_t calc_next_boundary(calendar_task_t *task, wall_time_t now);

// Math Logic API (Internal or Public)
wall_time_t sched_libev_next_periodic(wall_time_t offset, uint32_t interval_sec, wall_time_t now);

// Scheduler Executor API
void calendar_schedule(calendar_task_t *task);
void calendar_cancel(calendar_task_t *task);

#ifdef __cplusplus
}
#endif

#endif  // SCHED_TIME_H
