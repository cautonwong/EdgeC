#ifndef CORE_SCHED_H
#define CORE_SCHED_H

#include <stdint.h>
#include <time.h>
#include <uv.h>

typedef void (*TaskCb)(void *arg);

// Cron Task Handle
typedef struct {
  int id;
  int heap_index;      // Index in the heap array for fast removal/update
  time_t next_run_ts;  // Absolute timestamp for next run (the key for the heap)

  char cron_expr[64];

  TaskCb cb;
  void *arg;
} CronTask;

// Main Scheduler for Wall-Clock-Aware tasks
typedef struct {
  uv_loop_t *loop;
  uv_timer_t check_timer;  // The single 1-second tick timer

  // Min-Heap for efficient task retrieval
  CronTask **heap;
  size_t capacity;
  size_t size;

  // Clock jump detection
  time_t last_real_time;
} SchedulerV2;

/**
 * @brief Initializes the V2 scheduler.
 *
 * @param s The scheduler instance.
 * @param loop The libuv event loop.
 * @param max_tasks The pre-allocated capacity for cron tasks.
 * @return 0 on success, -1 on failure.
 */
int sched_v2_init(SchedulerV2 *s, uv_loop_t *loop, size_t max_tasks);

/**
 * @brief Adds a new cron-based task.
 *
 * @param s The scheduler instance.
 * @param id A user-defined ID for the task.
 * @param expr A standard cron expression string (e.g., "* * * * *").
 * @param cb The callback function to execute.
 * @param arg User data to pass to the callback.
 * @return 0 on success, -1 on failure (e.g., invalid expression).
 */
int sched_v2_add_cron(SchedulerV2 *s, int id, const char *expr, TaskCb cb,
                      void *arg);

/**
 * @brief Destroys the scheduler and frees all associated resources.
 *
 * @param s The scheduler instance.
 */
void sched_v2_destroy(SchedulerV2 *s);

#endif  // CORE_SCHED_H
