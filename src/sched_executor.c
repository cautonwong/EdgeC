#include <stdio.h>

#include "sched_time.h"

static void on_timer_fire(uv_timer_t *handle) {
    calendar_task_t *task = (calendar_task_t *)handle->data;
    if (!task) return;

    task->state = CAL_STATE_FIRED;
    if (task->on_fire) task->on_fire(task);

    task->persist.last_fired_wall = task->target_wall;
    task->state = CAL_STATE_DONE;
}

static void on_timer_close(uv_handle_t *handle) {
    // 句柄彻底释放，清空标记
    handle->data = NULL;
}

void calendar_schedule(calendar_task_t *task) {
    if (!task || !task->loop) return;

    wall_time_t wall_now = sched_wall_now();
    wall_time_t next_wall;

    if (task->type == CAL_TASK_PERIODIC_MATH) {
        next_wall = sched_libev_next_periodic(task->offset_wall, task->interval_sec, wall_now);
    } else {
        next_wall = calc_next_boundary(task, wall_now);
    }

    // 幂等检查
    if (task->persist.last_fired_wall >= next_wall) {
        task->state = CAL_STATE_DONE;
        return;
    }

    int64_t delta_ms = next_wall - wall_now;
    if (delta_ms < 0) delta_ms = 0;

    task->target_wall = next_wall;
    task->target_mono = sched_mono_now() + delta_ms;

    // libuv 安全模式：确保同一个内存块不被重复 init
    // 如果 handle 已激活或正在关闭，先 stop
    if (uv_is_active((uv_handle_t *)&task->timer)) {
        uv_timer_stop(&task->timer);
    }

    // 只有在从未初始化时才进行 init
    // 注意：libuv 没有直接判断是否 inited 的接口，我们通过 data 指针约定
    if (task->timer.data != task) {
        uv_timer_init(task->loop, &task->timer);
        task->timer.data = task;
    }

    uv_timer_start(&task->timer, on_timer_fire, (uint64_t)delta_ms, 0);
    task->state = CAL_STATE_SCHEDULED;
}

void calendar_cancel(calendar_task_t *task) {
    if (task && task->timer.data == task) {
        uv_timer_stop(&task->timer);
        if (!uv_is_closing((uv_handle_t *)&task->timer)) {
            uv_close((uv_handle_t *)&task->timer, on_timer_close);
        }
    }
}
