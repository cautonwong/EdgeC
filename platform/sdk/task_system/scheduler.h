#ifndef CEDGE_SCHEDULER_H
#define CEDGE_SCHEDULER_H

#include <uv.h>
#include <stdint.h>

// Forward declarations
typedef struct cedgex_task_t cedgex_task_t;
typedef struct cedgex_scheduler_t cedgex_scheduler_t;

// Callback function type for a task
typedef void (*cedgex_task_cb)(cedgex_scheduler_t *scheduler, cedgex_task_t *task, void *user_data);

// Represents a single periodic or delayed task
struct cedgex_task_t {
    int id;
    uint64_t interval_ms;     // For periodic tasks, 0 for one-shot
    uint64_t next_run_time_ms;  // Absolute time in ms for next execution
    cedgex_task_cb callback;
    void *user_data;
    int is_active;

    // Internal linked list pointers
    cedgex_task_t *next;
    cedgex_task_t *prev;
};

// The main scheduler structure
struct cedgex_scheduler_t {
    uv_loop_t *loop;
    uv_timer_t tick_timer;  // The single timer that drives the whole scheduler
    cedgex_task_t *task_list_head;
    int next_task_id;
};

/**
 * @brief Initializes the scheduler.
 *
 * @param scheduler The scheduler instance.
 * @param loop The libuv event loop to attach to.
 * @param tick_resolution_ms The resolution of the scheduler's internal timer.
 *        A smaller value (e.g., 10ms) provides higher accuracy but more CPU usage.
 * @return 0 on success, -1 on failure.
 */
int scheduler_init(cedgex_scheduler_t *scheduler, uv_loop_t *loop, uint64_t tick_resolution_ms);

/**
 * @brief Adds a new periodic task to the scheduler.
 *
 * @param scheduler The scheduler instance.
 * @param interval_ms The interval in milliseconds at which the task should run.
 * @param callback The function to call when the task executes.
 * @param user_data A pointer to user data that will be passed to the callback.
 * @return The ID of the new task, or -1 on failure.
 */
int scheduler_add_periodic_task(cedgex_scheduler_t *scheduler, uint64_t interval_ms, cedgex_task_cb callback, void *user_data);

/**
 * @brief Adds a new one-shot delayed task to the scheduler.
 *
 * @param scheduler The scheduler instance.
 * @param delay_ms The delay in milliseconds after which the task should run.
 * @param callback The function to call when the task executes.
 * @param user_data A pointer to user data that will be passed to the callback.
 * @return The ID of the new task, or -1 on failure.
 */
int scheduler_add_delayed_task(cedgex_scheduler_t *scheduler, uint64_t delay_ms, cedgex_task_cb callback, void *user_data);

/**
 * @brief Removes a task from the scheduler.
 *
 * @param scheduler The scheduler instance.
 * @param task_id The ID of the task to remove.
 * @return 0 on success, -1 if the task was not found.
 */
int scheduler_remove_task(cedgex_scheduler_t *scheduler, int task_id);

/**
 * @brief Stops the scheduler and cleans up resources.
 *
 * @param scheduler The scheduler instance.
 * @return 0 on success.
 */
int scheduler_cleanup(cedgex_scheduler_t *scheduler);

#endif // CEDGE_SCHEDULER_H
