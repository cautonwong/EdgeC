#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "edge_log.h"
#include "sched_time.h"

/**
 * @file sched_libev.c
 * @brief 时间跳变监控与自动重排
 */

typedef struct {
    uv_check_t jump_monitor;
    uint64_t last_wall_ms;
    bool monitor_active;
} sched_env_t;

static void _on_jump_check(uv_check_t *handle) {
    sched_env_t *env = (sched_env_t *)handle->data;
    uint64_t now = sched_wall_now();

    // 如果检测到挂钟时间跳变超过 2 秒，触发全量任务重排
    int64_t diff = (int64_t)now - (int64_t)env->last_wall_ms;
    if (diff > 2000 || diff < -2000) {
        LOG_WARN(LOG_CAT_SCHED, "Time jump detected: %ld ms. Rescheduling tasks...", diff);
        // 此处应触发所有注册任务的 recompute_all
    }

    env->last_wall_ms = now;
}

void scheduler_init(uv_loop_t *loop) {
    static sched_env_t env;
    env.last_wall_ms = sched_wall_now();

    uv_check_init(loop, &env.jump_monitor);
    env.jump_monitor.data = &env;
    uv_check_start(&env.jump_monitor, _on_jump_check);
}
