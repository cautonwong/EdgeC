#include <stdio.h>

#include "sched_time.h"

static void on_timer_fire(uv_timer_t *handle) {
    calendar_task_t *task = (calendar_task_t *)handle->data;

    // Check if we somehow missed the window significantly?
    // For now, assume soft real-time is sufficient.

    task->state = CAL_STATE_FIRED;

    if (task->on_fire) {
        task->on_fire(task);
    }

    // Update persistence state
    task->persist.last_fired_wall = task->target_wall;
    task->state = CAL_STATE_DONE;

    // In a real system, we might schedule the next one immediately here
    // or let the user re-schedule. For this primitive, it's one-shot per schedule call.
}

void calendar_schedule(calendar_task_t *task) {
    if (!task || !task->loop) return;

    wall_time_t wall_now = sched_wall_now();
    mono_time_t mono_now = sched_mono_now();

    wall_time_t next_wall = calc_next_boundary(task->type, wall_now);

    // Idempotency check: if we already fired for this boundary (or a later one), skip.
    if (task->persist.last_fired_wall >= next_wall) {
        task->state = CAL_STATE_DONE;
        return;
    }

    // Calculate delay based on Wall Clock difference
    int64_t delta_ms = next_wall - wall_now;

    // Safety clamp
    if (delta_ms < 0) {
        // Boundary is in the past? Should not happen with calc_next_boundary unless clock jumped
        // back JUST NOW. If so, execute immediately (0ms).
        delta_ms = 0;
    }

    // Map to Monotonic Target (for debugging/inspection)
    task->target_wall = next_wall;
    task->target_mono = mono_now + delta_ms;

    // Initialize/Start Timer
    // Note: In production, reuse the timer handle if possible or ensure it's closed properly.
    // For this design pattern, we assume 'task' lifecycle manages the timer.
    // We re-init to be safe or just start if already inited?
    // libuv allows uv_timer_start on active timer (updates it).
    // But we need to ensure it's inited. Let's assume caller manages lifecycle or we do lazy init.
    // Here we do lazy init assumption: task->timer should be usable.
    // But uv_timer_init is cheap.
    // ISSUE: Double init is bad if handle active.
    // We assume task->timer is part of the struct and managed by the task owner (init once).
    // BUT the example showed `uv_timer_init` inside schedule. This leaks handle if called
    // repeatedly without close. FIX: We will check if it's our first run or use a flag? Better:
    // Scheduler shouldn't init the handle every time. Refactoring: user calls scheduler_init(task),
    // then calendar_schedule(task). Or we just assume it's initialized? Let's stick to the prompt's
    // simplicity but add a safety check or init. Actually, `uv_timer_init` adds to loop. Repeated
    // calls on same memory = corruption. We will assume `uv_timer_init` is done ONCE by the user or
    // we track initialization. For this snippet, I will check `task->timer.data`. If NULL, init.
    // (Hack, but works for singleton tasks).

    if (task->timer.data != task) {
        uv_timer_init(task->loop, &task->timer);
        task->timer.data = task;
    }

    uv_timer_start(&task->timer, on_timer_fire, delta_ms, 0);
    task->state = CAL_STATE_SCHEDULED;
}

static void on_timer_close(uv_handle_t *handle) {
    // 标记句柄已彻底关闭
    handle->data = NULL;
}

void calendar_cancel(calendar_task_t *task) {
    if (!task || !task->timer.data) return;

    uv_timer_stop(&task->timer);
    if (!uv_is_closing((uv_handle_t *)&task->timer)) {
        uv_close((uv_handle_t *)&task->timer, on_timer_close);
    }
}
