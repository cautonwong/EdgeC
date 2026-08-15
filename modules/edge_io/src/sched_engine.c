#include <math.h>
#include <sys/time.h>
#include <time.h>

#include "sched_time.h"

static double _get_now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

static void _uv_periodic_internal_cb(uv_timer_t* timer) {
    uv_periodic_t* handle = (uv_periodic_t*)timer->data;

    // 1. 先执行业务逻辑
    if (handle->cb) handle->cb(handle);

    // 2. 计算下一次触发时间 (模仿 libev)
    double now = _get_now_ms();
    double next_at;

    if (handle->reschedule_cb) {
        // [模仿 libev] 输入 ms, 输出 ms
        next_at = handle->reschedule_cb(handle, now);
    } else if (handle->interval_ms > 0) {
        next_at = now + handle->interval_ms;
    } else {
        return;  // 一次性任务
    }

    handle->target_wall = next_at;

    // 3. 重新 arm timer
    double delay = next_at - now;
    uint64_t timeout_ms = (delay > 0) ? (uint64_t)delay : 1;

    uv_timer_start(&handle->timer, _uv_periodic_internal_cb, timeout_ms, 0);
}

void uv_periodic_init(uv_loop_t* loop, uv_periodic_t* handle) {
    uv_timer_init(loop, &handle->timer);
    handle->timer.data = handle;
    handle->reschedule_cb = NULL;
    handle->offset_ms = 0;
    handle->interval_ms = 0;
    handle->target_wall = 0;
}

void uv_periodic_set(uv_periodic_t* handle, double offset_ms, double interval_ms,
                     uv_periodic_reschedule_cb resched_cb) {
    handle->offset_ms = offset_ms;
    handle->interval_ms = interval_ms;
    handle->reschedule_cb = resched_cb;
}

void uv_periodic_start(uv_periodic_t* handle, uv_periodic_cb cb) {
    handle->cb = cb;
    double now = _get_now_ms();
    handle->target_wall = now + handle->offset_ms;

    double delay = handle->target_wall - now;
    uint64_t timeout_ms = (delay > 0) ? (uint64_t)delay : 1;
    uv_timer_start(&handle->timer, _uv_periodic_internal_cb, timeout_ms, 0);
}

void uv_periodic_stop(uv_periodic_t* handle) {
    uv_timer_stop(&handle->timer);
}

// 通用对齐策略：从 baseline 开始，每 interval_sec 秒触发一次
static double _calculate_next_align(double now_ms, double baseline_offset_sec,
                                    double interval_sec) {
    double now_sec = now_ms / 1000.0;
    // 找到今天 00:00:00 UTC
    double day_start = floor(now_sec / 86400.0) * 86400.0;
    double anchor = day_start + baseline_offset_sec;

    // 如果 anchor 已经在很远的过去，调整到当前时间附近以防溢出
    if (now_sec - anchor > 1000000) {
        anchor = now_sec - fmod(now_sec - anchor, interval_sec);
    }

    double elapsed = now_sec - anchor;
    double periods = floor(elapsed / interval_sec) + 1.0;
    return (anchor + periods * interval_sec) * 1000.0;
}

double sched_policy_14h_from_000030(uv_periodic_t* handle, double now_ms) {
    (void)handle;
    return _calculate_next_align(now_ms, 30.0, 14.0 * 3600.0);
}

// 示例策略：对齐到分钟 (输入 ms, 输出 ms)
double sched_policy_at_minute(uv_periodic_t* handle, double now_ms) {
    (void)handle;
    double now_sec = now_ms / 1000.0;
    return (floor(now_sec / 60.0) * 60.0 + 60.0) * 1000.0;
}

double sched_policy_at_hour(uv_periodic_t* handle, double now_ms) {
    (void)handle;
    double now_sec = now_ms / 1000.0;
    return (floor(now_sec / 3600.0) * 3600.0 + 3600.0) * 1000.0;
}
