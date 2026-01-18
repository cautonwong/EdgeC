#include <stdbool.h>
#include <stdlib.h>
#include <uv.h>

#include "edge_log.h"
#include "sched_time.h"

/**
 * @file sched_engine.c
 * @brief 统一调度引擎 (整合执行、算法与监控)
 */

// --- 1. 内部状态管理 ---
static struct uv__queue active_tasks;
static int initialized = 0;

typedef struct {
    uv_check_t jump_monitor;
    uint64_t last_wall_ms;
} sched_env_t;

static sched_env_t g_env;

static void ensure_init(void) {
    if (!initialized) {
        uv__queue_init(&active_tasks);
        initialized = 1;
    }
}

// --- 2. 调度算法 (借鉴 ev_periodic) ---

static wall_time_t get_day_start_ms(wall_time_t now_ms) {
    time_t sec = (time_t)(now_ms / 1000);
    struct tm t;
    gmtime_r(&sec, &t);
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    return (wall_time_t)timegm(&t) * 1000;
}

wall_time_t sched_policy_aligned(struct uv_periodic_s *handle, wall_time_t now) {
    if (handle->interval_ms == 0) {
        return (handle->offset_ms > now) ? handle->offset_ms : now + 1000;
    }
    int64_t d = (int64_t)now - (int64_t)handle->offset_ms;
    if (d < 0) return handle->offset_ms;
    uint64_t count = (uint64_t)d / handle->interval_ms + 1;
    return handle->offset_ms + count * handle->interval_ms;
}

wall_time_t sched_policy_at_minute(struct uv_periodic_s *handle, wall_time_t now) {
    wall_time_t day_start = get_day_start_ms(now);
    struct uv_periodic_s tmp = {.offset_ms = day_start + (handle->offset_ms % 60000),
                                .interval_ms = 60000};
    wall_time_t res = sched_policy_aligned(&tmp, now);
    if (res <= now) {
        tmp.offset_ms += 86400000;
        res = sched_policy_aligned(&tmp, now);
    }
    return res;
}

wall_time_t sched_policy_at_hour(struct uv_periodic_s *handle, wall_time_t now) {
    wall_time_t day_start = get_day_start_ms(now);
    struct uv_periodic_s tmp = {.offset_ms = day_start + (handle->offset_ms % 3600000),
                                .interval_ms = 3600000};
    wall_time_t res = sched_policy_aligned(&tmp, now);
    if (res <= now) {
        tmp.offset_ms += 86400000;
        res = sched_policy_aligned(&tmp, now);
    }
    return res;
}

// --- 3. 执行器逻辑 ---

static void on_timer_fire(uv_timer_t *timer_handle) {
    uv_periodic_t *handle = (uv_periodic_t *)timer_handle->data;
    if (uv_is_closing((uv_handle_t *)timer_handle)) return;

    if (handle->cb) handle->cb(handle);

    wall_time_t now = sched_wall_now();
    handle->target_wall = handle->reschedule_cb ? handle->reschedule_cb(handle, now)
                                                : sched_policy_aligned(handle, now);

    uint64_t delay = (handle->target_wall > now) ? (handle->target_wall - now) : 100;
    if (!uv_is_closing((uv_handle_t *)timer_handle))
        uv_timer_start(&handle->timer, on_timer_fire, delay, 0);
}

void uv_periodic_start(uv_periodic_t *handle, uv_periodic_cb cb) {
    if (!handle || !handle->loop) return;
    ensure_init();

    handle->cb = cb;
    if (handle->node.next == NULL) uv__queue_init(&handle->node);
    if (uv__queue_empty(&handle->node)) uv__queue_insert_tail(&active_tasks, &handle->node);

    wall_time_t now = sched_wall_now();
    handle->target_wall = handle->reschedule_cb ? handle->reschedule_cb(handle, now)
                                                : sched_policy_aligned(handle, now);

    if (handle->timer.type != UV_TIMER) {
        uv_timer_init(handle->loop, &handle->timer);
        handle->timer.data = handle;
    }
    uint64_t delay = (handle->target_wall > now) ? (handle->target_wall - now) : 1;
    uv_timer_start(&handle->timer, on_timer_fire, delay, 0);
}

void uv_periodic_stop(uv_periodic_t *handle) {
    if (!handle) return;
    if (handle->node.next != NULL && !uv__queue_empty(&handle->node)) {
        uv__queue_remove(&handle->node);
        uv__queue_init(&handle->node);  // Reset
    }
    if (uv_is_active((uv_handle_t *)&handle->timer)) uv_timer_stop(&handle->timer);
    if (!uv_is_closing((uv_handle_t *)&handle->timer))
        uv_close((uv_handle_t *)&handle->timer, NULL);
}

void scheduler_reschedule_all(void) {
    if (!initialized) return;
    struct uv__queue *q;
    wall_time_t now = sched_wall_now();
    int count = 0;
    uv__queue_foreach(q, &active_tasks) {
        uv_periodic_t *handle = uv__queue_data(q, uv_periodic_t, node);
        handle->target_wall = handle->reschedule_cb ? handle->reschedule_cb(handle, now)
                                                    : sched_policy_aligned(handle, now);
        uint64_t delay = (handle->target_wall > now) ? (handle->target_wall - now) : 1;
        if (!uv_is_closing((uv_handle_t *)&handle->timer))
            uv_timer_start(&handle->timer, on_timer_fire, delay, 0);
        count++;
    }
    LOG_INFO(LOG_CAT_SCHED, "Rescheduled %d periodic handles due to time jump.", count);
}

// --- 4. 监控逻辑 ---

static void _on_jump_check(uv_check_t *handle) {
    sched_env_t *env = (sched_env_t *)handle->data;
    uint64_t now = sched_wall_now();
    int64_t diff = (int64_t)now - (int64_t)env->last_wall_ms;
    if (diff > 2000 || diff < -2000) {
        LOG_WARN(LOG_CAT_SCHED, "Time jump detected: %ld ms. Rescheduling...", diff);
        scheduler_reschedule_all();
    }
    env->last_wall_ms = now;
}

void scheduler_init(uv_loop_t *loop) {
    g_env.last_wall_ms = sched_wall_now();
    uv_check_init(loop, &g_env.jump_monitor);
    g_env.jump_monitor.data = &g_env;
    uv_check_start(&g_env.jump_monitor, _on_jump_check);
}
