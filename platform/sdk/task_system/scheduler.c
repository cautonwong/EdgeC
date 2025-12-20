#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>

// The core tick function that checks and runs tasks.
static void scheduler_tick(uv_timer_t *handle) {
    cedgex_scheduler_t *scheduler = (cedgex_scheduler_t*)handle->data;
    if (!scheduler) return;

    uint64_t now_ms = uv_now(scheduler->loop);

    cedgex_task_t *task = scheduler->task_list_head;
    while (task != NULL) {
        // Grab next task now, in case the current one gets removed in the callback
        cedgex_task_t *next_task = task->next;

        if (task->is_active && (now_ms >= task->next_run_time_ms)) {
            // Execute the task's callback
            if (task->callback) {
                task->callback(scheduler, task, task->user_data);
            }

            // If it's a periodic task, reschedule it. Otherwise, deactivate it.
            if (task->interval_ms > 0) {
                task->next_run_time_ms = now_ms + task->interval_ms;
            } else {
                // This was a one-shot task, mark it for removal
                task->is_active = 0;
            }
        }
        task = next_task;
    }
}

int scheduler_init(cedgex_scheduler_t *scheduler, uv_loop_t *loop, uint64_t tick_resolution_ms) {
    if (!scheduler || !loop) return -1;

    scheduler->loop = loop;
    scheduler->task_list_head = NULL;
    scheduler->next_task_id = 1;

    uv_timer_init(loop, &scheduler->tick_timer);
    scheduler->tick_timer.data = scheduler;
    
    // Start the master timer
    uv_timer_start(&scheduler->tick_timer, scheduler_tick, tick_resolution_ms, tick_resolution_ms);
    
    return 0;
}

static int scheduler_add_task(cedgex_scheduler_t *scheduler, uint64_t initial_delay_ms, uint64_t interval_ms, cedgex_task_cb callback, void *user_data) {
    if (!scheduler || !callback) return -1;

    cedgex_task_t *new_task = (cedgex_task_t*)malloc(sizeof(cedgex_task_t));
    if (!new_task) return -1;

    new_task->id = scheduler->next_task_id++;
    new_task->interval_ms = interval_ms;
    new_task->next_run_time_ms = uv_now(scheduler->loop) + initial_delay_ms;
    new_task->callback = callback;
    new_task->user_data = user_data;
    new_task->is_active = 1;
    new_task->next = NULL;
    new_task->prev = NULL;

    // Add to the head of the list
    if (scheduler->task_list_head == NULL) {
        scheduler->task_list_head = new_task;
    } else {
        new_task->next = scheduler->task_list_head;
        scheduler->task_list_head->prev = new_task;
        scheduler->task_list_head = new_task;
    }

    return new_task->id;
}

int scheduler_add_periodic_task(cedgex_scheduler_t *scheduler, uint64_t interval_ms, cedgex_task_cb callback, void *user_data) {
    // A periodic task runs first after one interval
    return scheduler_add_task(scheduler, interval_ms, interval_ms, callback, user_data);
}

int scheduler_add_delayed_task(cedgex_scheduler_t *scheduler, uint64_t delay_ms, cedgex_task_cb callback, void *user_data) {
    // A delayed task is a one-shot task (interval = 0)
    return scheduler_add_task(scheduler, delay_ms, 0, callback, user_data);
}

int scheduler_remove_task(cedgex_scheduler_t *scheduler, int task_id) {
    if (!scheduler) return -1;

    cedgex_task_t *task = scheduler->task_list_head;
    while (task != NULL) {
        if (task->id == task_id) {
            // Unlink from the list
            if (task->prev) {
                task->prev->next = task->next;
            } else { // It's the head
                scheduler->task_list_head = task->next;
            }
            if (task->next) {
                task->next->prev = task->prev;
            }

            free(task);
            return 0; // Success
        }
        task = task->next;
    }

    return -1; // Not found
}

int scheduler_cleanup(cedgex_scheduler_t *scheduler) {
    if (!scheduler) return -1;

    // Stop the timer
    uv_timer_stop(&scheduler->tick_timer);

    // Free all tasks
    cedgex_task_t *task = scheduler->task_list_head;
    while (task != NULL) {
        cedgex_task_t *next_task = task->next;
        free(task);
        task = next_task;
    }
    scheduler->task_list_head = NULL;

    return 0;
}
