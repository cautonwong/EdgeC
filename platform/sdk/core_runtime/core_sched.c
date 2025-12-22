#include "core_sched.h"
#include "ccronexpr.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct cron_task {
    cron_expr expr;
    time_t next_run;
    sched_cb cb;
    void *arg;
    int heap_idx;
};

typedef struct {
    uv_loop_t *loop;
    int timer_fd;
    uv_poll_t poll_handle;
    cron_task_t **heap;
    size_t capacity;
    size_t size;
    time_t last_run_time;
} scheduler_t;

static scheduler_t g_sched;

// --- Min-Heap Implementation ---
static void _swap(int i, int j) {
    cron_task_t *tmp = g_sched.heap[i];
    g_sched.heap[i] = g_sched.heap[j];
    g_sched.heap[j] = tmp;
    g_sched.heap[i]->heap_idx = i;
    g_sched.heap[j]->heap_idx = j;
}

static void _heap_up(int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (g_sched.heap[i]->next_run < g_sched.heap[parent]->next_run) {
            _swap(i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

static void _heap_down(int i) {
    while (2 * i + 1 < g_sched.size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = left;
        if (right < g_sched.size && g_sched.heap[right]->next_run < g_sched.heap[left]->next_run) {
            smallest = right;
        }
        if (g_sched.heap[i]->next_run > g_sched.heap[smallest]->next_run) {
            _swap(i, smallest);
            i = smallest;
        } else {
            break;
        }
    }
}

static void _heap_push(cron_task_t *task) {
    if (g_sched.size >= g_sched.capacity) return;
    task->heap_idx = g_sched.size;
    g_sched.heap[g_sched.size++] = task;
    _heap_up(g_sched.size - 1);
}

static cron_task_t* _heap_pop() {
    if (g_sched.size == 0) return NULL;
    cron_task_t *root = g_sched.heap[0];
    g_sched.heap[0] = g_sched.heap[--g_sched.size];
    if (g_sched.size > 0) {
        g_sched.heap[0]->heap_idx = 0;
        _heap_down(0);
    }
    return root;
}
// --- End Min-Heap ---

static void _rearm_timer() {
    cron_task_t *top = (g_sched.size > 0) ? g_sched.heap[0] : NULL;
    struct itimerspec new_val = {0};
    if (top) {
        new_val.it_value.tv_sec = top->next_run;
    }
    int flags = TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET;
    timerfd_settime(g_sched.timer_fd, flags, &new_val, NULL);
}

static void _on_timer_event(uv_poll_t *handle, int status, int events) {
    uint64_t exp;
    ssize_t sz = read(g_sched.timer_fd, &exp, sizeof(uint64_t));
    time_t now = time(NULL);

    if (sz != sizeof(uint64_t)) {
        printf("[Sched] Clock jump detected! Recalculating...\n");
        for (size_t i = 0; i < g_sched.size; i++) {
            g_sched.heap[i]->next_run = cron_next(&g_sched.heap[i]->expr, now);
        }
        for (int i = (g_sched.size / 2) - 1; i >= 0; i--) {
            _heap_down(i);
        }
    } else {
        while (g_sched.size > 0) {
            cron_task_t *t = g_sched.heap[0];
            if (t->next_run > now) break;
            
            _heap_pop();
            if (t->next_run > g_sched.last_run_time && t->cb) {
                t->cb(t->arg);
            }
            t->next_run = cron_next(&t->expr, now);
            _heap_push(t);
        }
    }
    g_sched.last_run_time = now;
    _rearm_timer();
}

int core_sched_init(uv_loop_t *loop, size_t max_tasks) {
    memset(&g_sched, 0, sizeof(g_sched));
    g_sched.loop = loop;
    g_sched.capacity = max_tasks;
    g_sched.heap = calloc(max_tasks, sizeof(cron_task_t*));
    if (!g_sched.heap) return -1;

    g_sched.timer_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if (g_sched.timer_fd < 0) {
        free(g_sched.heap);
        return -1;
    }
    
    uv_poll_init(loop, &g_sched.poll_handle, g_sched.timer_fd);
    uv_poll_start(&g_sched.poll_handle, UV_READABLE, _on_timer_event);
    return 0;
}

cron_task_t* core_sched_add_cron(const char *cron_str, sched_cb cb, void *arg) {
    const char *err = NULL;
    cron_task_t *task = calloc(1, sizeof(cron_task_t));
    if (!task) return NULL;

    cron_parse_expr(cron_str, &task->expr, &err);
    if (err) {
        free(task);
        return NULL;
    }

    task->cb = cb;
    task->arg = arg;
    task->next_run = cron_next(&task->expr, time(NULL));
    
    _heap_push(task);
    _rearm_timer();
    
    return task;
}

void core_sched_destroy(void) {
    uv_poll_stop(&g_sched.poll_handle);
    close(g_sched.timer_fd);
    for (size_t i = 0; i < g_sched.size; i++) {
        free(g_sched.heap[i]);
    }
    free(g_sched.heap);
}
