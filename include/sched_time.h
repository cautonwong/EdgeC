#ifndef SCHED_TIME_H
#define SCHED_TIME_H

#include <stdint.h>
#include <time.h>
#include <uv.h>

#include "queue.h"

/**
 * @file sched_time.h
 * @brief 调度平面公共头文件 (支持 DCU 周期对齐与 S10 守时)
 */

typedef uint64_t wall_time_t;
typedef uint64_t mono_time_t;

struct uv_periodic_s;

/**
 * @brief 重排策略回调 (仿照 ev_periodic_reschedule_cb)
 * @param handle 周期任务句柄
 * @param now 当前挂钟时间 (ms)
 * @return 下一次触发的挂钟时间 (ms)
 */
typedef wall_time_t (*uv_periodic_reschedule_cb)(struct uv_periodic_s *handle, wall_time_t now);

/**
 * @brief 任务触发回调
 */
typedef void (*uv_periodic_cb)(struct uv_periodic_s *handle);

/**
 * @brief 挂钟调度任务结构 (uv_periodic 风格)
 */
typedef struct uv_periodic_s {
    uv_timer_t timer;
    struct uv__queue node;

    // 核心参数 (Base + Interval)
    wall_time_t offset_ms;    // 基础偏移 (ev_periodic: offset)
    wall_time_t interval_ms;  // 周期间隔 (ev_periodic: interval)

    // 策略回调 (如果为 NULL，则使用默认的 interval 逻辑)
    uv_periodic_reschedule_cb reschedule_cb;

    wall_time_t target_wall;  // 运行时计算出的下一次目标

    uv_periodic_cb cb;
    void *data;

    uv_loop_t *loop;
} uv_periodic_t;

// 标准策略实现 (预置算法)
wall_time_t sched_policy_aligned(struct uv_periodic_s *handle, wall_time_t now);    // 通用对齐
wall_time_t sched_policy_at_minute(struct uv_periodic_s *handle, wall_time_t now);  // 整分
wall_time_t sched_policy_at_hour(struct uv_periodic_s *handle, wall_time_t now);    // 整时

// 物理接口
wall_time_t sched_wall_now(void);
mono_time_t sched_mono_now(void);
void uv_periodic_start(uv_periodic_t *handle, uv_periodic_cb cb);
void uv_periodic_stop(uv_periodic_t *handle);
void scheduler_reschedule_all(void);

#endif