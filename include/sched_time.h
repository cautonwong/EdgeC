#ifndef SCHED_TIME_H
#define SCHED_TIME_H

#include <stdint.h>
#include <uv.h>

typedef struct uv_periodic_s uv_periodic_t;

/**
 * @brief 模仿 libev 的重新调度回调
 * @param handle
 * @param now 当前系统绝对时间戳 (秒)
 * @return 下一次触发的绝对时间戳 (秒)
 */
typedef double (*uv_periodic_reschedule_cb)(uv_periodic_t* handle, double now);

/**
 * @brief 业务触发回调
 */
typedef void (*uv_periodic_cb)(uv_periodic_t* handle);

struct uv_periodic_s {
    uv_timer_t timer; /**< 内部利用 libuv timer 实现 */

    uv_periodic_cb cb; /**< 业务逻辑 */

    uv_periodic_reschedule_cb reschedule_cb; /**< 模仿 libev 的策略回调 */

    double offset_ms; /**< 初始偏移量 (ms) */

    double interval_ms; /**< 固定周期 (ms) */

    double target_wall; /**< 目标挂钟时间点 (ms) */

    void* data; /**< 用户上下文 */
};

/**
 * @brief 初始化周期性调度器
 */
void uv_periodic_init(uv_loop_t* loop, uv_periodic_t* handle);

/**
 * @brief 模仿 libev 的 ev_periodic_set
 */
void uv_periodic_set(uv_periodic_t* handle, double offset, double interval,
                     uv_periodic_reschedule_cb resched_cb);

/**
 * @brief 启动调度
 */
void uv_periodic_start(uv_periodic_t* handle, uv_periodic_cb cb);

/**
 * @brief 停止调度
 */
void uv_periodic_stop(uv_periodic_t* handle);

/* --- 标准策略库 (模仿 libev 常见模式) --- */
double sched_policy_at_minute(uv_periodic_t* handle, double now);
double sched_policy_at_hour(uv_periodic_t* handle, double now);

#endif
