#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sched_time.h"

#define CLOCK_JUMP_THRESHOLD 2000  // 2秒以上的偏差视为跳变

static struct {
    wall_time_t last_wall;
    mono_time_t last_mono;
    uv_check_t jump_monitor;
    bool monitor_active;
} g_sched_env = {0};

static void _recompute_all_tasks(uv_loop_t *loop) {
    (void)loop;
    printf("[Sched] System clock jump detected! Re-synchronizing tasks...\n");
}

static void _on_jump_check(uv_check_t *handle) {
    wall_time_t wall_now = sched_wall_now();
    mono_time_t mono_now = sched_mono_now();

    if (g_sched_env.last_wall > 0) {
        int64_t wall_diff = wall_now - g_sched_env.last_wall;
        int64_t mono_diff = mono_now - g_sched_env.last_mono;

        if (abs((int)(wall_diff - mono_diff)) > CLOCK_JUMP_THRESHOLD) {
            _recompute_all_tasks(handle->loop);
        }
    }

    g_sched_env.last_wall = wall_now;
    g_sched_env.last_mono = mono_now;
}

void scheduler_init(uv_loop_t *loop) {
    if (g_sched_env.monitor_active || !loop) return;

    uv_check_init(loop, &g_sched_env.jump_monitor);
    uv_check_start(&g_sched_env.jump_monitor, _on_jump_check);
    g_sched_env.monitor_active = true;
}

wall_time_t sched_libev_next_periodic(wall_time_t offset, uint32_t interval_sec, wall_time_t now) {
    if (interval_sec == 0) return offset > now ? offset : (now + 1);

    double interval_ms = interval_sec * 1000.0;
    double diff = (double)now - (double)offset;

    if (diff < 0) return offset;

    double n = ceil(diff / interval_ms);
    // libev logic: next = offset + n * interval.
    // If now falls exactly on boundary, libev often goes to next.
    if (offset + (wall_time_t)(n * interval_ms) <= now) {
        n += 1.0;
    }

    return offset + (wall_time_t)(n * interval_ms);
}
