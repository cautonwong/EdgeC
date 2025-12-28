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
    CAL_TASK_MONTH_FREEZE
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

    calendar_persist_t persist;

    void (*on_fire)(struct calendar_task *task);
} calendar_task_t;

// Time Source API (Mockable)
wall_time_t sched_wall_now(void);
mono_time_t sched_mono_now(void);

// Calendar Logic API
wall_time_t calc_next_boundary(calendar_task_type_t type, wall_time_t now);

// Scheduler Executor API
void calendar_schedule(calendar_task_t *task);
void calendar_cancel(calendar_task_t *task);

#ifdef __cplusplus
}
#endif

#endif  // SCHED_TIME_H
