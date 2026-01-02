#include <stdlib.h>
#include <uv.h>

#include "edge_log.h"
#include "sched_time.h"

/**
 * @file sched_executor.c
 * @brief 挂钟调度执行器 (生产加固版)
 */

static void on_timer_fire(uv_timer_t *handle) {
    calendar_task_t *task = (calendar_task_t *)handle->data;

    // 如果 Handle 正在关闭，严禁重武装
    if (uv_is_closing((uv_handle_t *)handle)) return;

    if (task->on_fire) task->on_fire(task);

    // 计算下一次对齐点
    wall_time_t now = sched_wall_now();
    task->target_wall = calc_next_boundary(task, now);

    // 工业定力：确证延迟至少为 100ms，防止 Mock 环境下的高频风暴
    uint64_t delay = (task->target_wall > now) ? (task->target_wall - now) : 100;

    if (!uv_is_closing((uv_handle_t *)handle)) {
        uv_timer_start(&task->timer, on_timer_fire, delay, 0);
    }
}

void calendar_schedule(calendar_task_t *task) {
    if (!task || !task->loop) return;

    wall_time_t now = sched_wall_now();
    task->target_wall = calc_next_boundary(task, now);

    // 只有在未初始化时才 init，防止重复 init 导致的 libuv 内部崩溃
    if (task->timer.type != UV_TIMER) {
        uv_timer_init(task->loop, &task->timer);
        task->timer.data = task;
    }

    uint64_t delay = (task->target_wall > now) ? (task->target_wall - now) : 1;
    uv_timer_start(&task->timer, on_timer_fire, delay, 0);
}

void calendar_cancel(calendar_task_t *task) {
    if (!task) return;
    if (uv_is_active((uv_handle_t *)&task->timer)) {
        uv_timer_stop(&task->timer);
    }
    if (!uv_is_closing((uv_handle_t *)&task->timer)) {
        // 注意：此处不应物理 free，因为 task 可能在栈上或由外部管理
        uv_close((uv_handle_t *)&task->timer, NULL);
    }
}
