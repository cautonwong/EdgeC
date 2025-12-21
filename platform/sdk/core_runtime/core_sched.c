#include "core_sched.h"
#include "ccronexpr.h" 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// =========================================================
// 1. Min-Heap Algorithm (O(log N)) for task management
// =========================================================

static void _swap_tasks(SchedulerV2 *s, int i, int j) {
    CronTask *temp = s->heap[i];
    s->heap[i] = s->heap[j];
    s->heap[j] = temp;
    
    // Update the index stored within the task itself for fast access
    s->heap[i]->heap_index = i;
    s->heap[j]->heap_index = j;
}

static void _heap_sift_up(SchedulerV2 *s, int idx) {
    while (idx > 0) {
        int parent_idx = (idx - 1) / 2;
        if (s->heap[idx]->next_run_ts < s->heap[parent_idx]->next_run_ts) {
            _swap_tasks(s, idx, parent_idx);
            idx = parent_idx;
        } else {
            break;
        }
    }
}

static void _heap_sift_down(SchedulerV2 *s, int idx) {
    int left_child_idx;
    while ((left_child_idx = 2 * idx + 1) < s->size) {
        int right_child_idx = left_child_idx + 1;
        int smallest_child_idx = left_child_idx;
        
        if (right_child_idx < s->size && s->heap[right_child_idx]->next_run_ts < s->heap[left_child_idx]->next_run_ts) {
            smallest_child_idx = right_child_idx;
        }
        
        if (s->heap[idx]->next_run_ts > s->heap[smallest_child_idx]->next_run_ts) {
            _swap_tasks(s, idx, smallest_child_idx);
            idx = smallest_child_idx;
        } else {
            break;
        }
    }
}

static void _heap_push(SchedulerV2 *s, CronTask *task) {
    if (s->size >= s->capacity) {
        fprintf(stderr, "[SchedV2] Error: Scheduler capacity exceeded.\n");
        return;
    }
    task->heap_index = s->size;
    s->heap[s->size++] = task;
    _heap_sift_up(s, s->size - 1);
}

static CronTask* _heap_pop(SchedulerV2 *s) {
    if (s->size == 0) return NULL;
    CronTask *root = s->heap[0];
    
    s->heap[0] = s->heap[s->size - 1];
    s->size--;

    if (s->size > 0) {
        s->heap[0]->heap_index = 0;
        _heap_sift_down(s, 0);
    }
    return root;
}

// =========================================================
// 2. Cron Logic and Clock-Jump-Aware Tick
// =========================================================

static time_t _calc_next_run_time(const char *expr, time_t now) {
    cron_expr parsed;
    const char *err = NULL;
    cron_parse_expr(expr, &parsed, &err);
    if (err) {
        fprintf(stderr, "[SchedV2] Invalid cron expression '%s': %s\n", expr, err);
        return 0;
    }
    return cron_next(&parsed, now);
}

static void _on_scheduler_tick(uv_timer_t *handle) {
    SchedulerV2 *s = (SchedulerV2 *)handle->data;
    time_t now = time(NULL);
    
    // --- Clock Jump Detection ---
    if (labs(now - s->last_real_time) > 2) { // Use labs for long int
        printf("[SchedV2] WARN: Clock jump detected! Last seen: %ld, Now: %ld. Recalculating schedule...\n", s->last_real_time, now);
        
        // Re-calculate all run times based on the new "now"
        for (size_t i = 0; i < s->size; i++) {
            CronTask *t = s->heap[i];
            t->next_run_ts = _calc_next_run_time(t->cron_expr, now);
        }
        // Re-heapify the entire array, which is an O(N) operation.
        for (int i = (s->size / 2) - 1; i >= 0; i--) {
            _heap_sift_down(s, i);
        }
    }
    s->last_real_time = now;

    // --- Execute Due Tasks ---
    while (s->size > 0) {
        CronTask *t = s->heap[0]; // Peek at the task with the earliest run time
        
        if (t->next_run_ts <= now) {
            _heap_pop(s); // Remove it from the heap
            
            if (t->cb) {
                t->cb(t->arg); // Execute the callback
            }
            
            // Re-calculate its next run time and push it back into the heap
            time_t next_ts = _calc_next_run_time(t->cron_expr, now);
            if (next_ts > 0) {
                t->next_run_ts = next_ts;
                _heap_push(s, t);
            } else {
                // Invalid expression or one-shot, free the task
                free(t);
            }
        } else {
            // The earliest task is not due yet, so no other tasks can be.
            break;
        }
    }
}

// =========================================================
// 3. Public API Implementation
// =========================================================

int sched_v2_init(SchedulerV2 *s, uv_loop_t *loop, size_t max_tasks) {
    if (!s || !loop) return -1;
    s->loop = loop;
    s->capacity = max_tasks;
    s->size = 0;
    s->heap = calloc(max_tasks, sizeof(CronTask*));
    if (!s->heap) return -1;
    
    s->last_real_time = time(NULL);
    
    uv_timer_init(loop, &s->check_timer);
    s->check_timer.data = s;
    uv_timer_start(&s->check_timer, _on_scheduler_tick, 1000, 1000); // 1-second tick
    
    return 0;
}

int sched_v2_add_cron(SchedulerV2 *s, int id, const char *expr, TaskCb cb, void *arg) {
    if (!s || !expr || !cb) return -1;

    CronTask *t = calloc(1, sizeof(CronTask));
    if (!t) return -1;

    t->id = id;
    strncpy(t->cron_expr, expr, sizeof(t->cron_expr) - 1);
    t->cb = cb;
    t->arg = arg;
    
    time_t now = time(NULL);
    t->next_run_ts = _calc_next_run_time(expr, now);
    
    if (t->next_run_ts == 0) {
        free(t);
        return -1; // Invalid cron expression
    }
    
    _heap_push(s, t);
    return 0;
}

void sched_v2_destroy(SchedulerV2 *s) {
    if (!s) return;
    uv_timer_stop(&s->check_timer);
    for (size_t i = 0; i < s->size; i++) {
        free(s->heap[i]);
    }
    free(s->heap);
    s->size = 0;
    s->capacity = 0;
}
