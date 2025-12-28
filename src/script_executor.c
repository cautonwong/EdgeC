#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "js_engine.h"
#include "svc_script.h"

#define MAX_QUEUE_SIZE 128
#define MAX_SCRIPTS 64

typedef enum {
    SCRIPT_STATE_NORMAL,
    SCRIPT_STATE_FROZEN,
    SCRIPT_STATE_PROBE,
    SCRIPT_STATE_DISABLED
} script_state_t;

typedef struct {
    char id[32];
    script_state_t state;
    int error_count;
    uint64_t frozen_until;
} script_stat_t;

typedef struct {
    char script_id[32];
    script_prio_t prio;
    PipeMsg input;
} script_task_t;

struct script_exec_s {
    uv_thread_t tid;
    uv_loop_t compute_loop;
    uv_async_t task_async;
    uv_async_t stop_async;
    js_engine_t *engine;
    uv_mutex_t lock;
    script_task_t queue[MAX_QUEUE_SIZE];
    int head, tail;
    script_stat_t scripts[MAX_SCRIPTS];
    int script_count;
    volatile bool running;
};

static script_stat_t *_get_script_stat(script_executor_t *exec, const char *id) {
    for (int i = 0; i < exec->script_count; i++) {
        if (strcmp(exec->scripts[i].id, id) == 0) return &exec->scripts[i];
    }
    if (exec->script_count < MAX_SCRIPTS) {
        script_stat_t *s = &exec->scripts[exec->script_count++];
        strncpy(s->id, id, 31);
        s->state = SCRIPT_STATE_NORMAL;
        return s;
    }
    return NULL;
}

static bool _check_can_run(script_stat_t *stat) {
    if (!stat) return true;
    uint64_t now = uv_hrtime() / 1000000;
    if (stat->state == SCRIPT_STATE_DISABLED) return false;
    if (stat->state == SCRIPT_STATE_FROZEN) {
        if (now < stat->frozen_until) return false;
        stat->state = SCRIPT_STATE_PROBE;
    }
    return true;
}

static void _report_result(script_stat_t *stat, int ret) {
    if (!stat) return;
    if (ret == 0) {
        if (stat->state == SCRIPT_STATE_PROBE) {
            stat->state = SCRIPT_STATE_NORMAL;
            stat->error_count = 0;
        }
    } else {
        stat->error_count++;
        if (stat->error_count > 5) {
            stat->state = SCRIPT_STATE_FROZEN;
            stat->frozen_until = (uv_hrtime() / 1000000) + 60000;
        }
    }
}

static void _process_tasks(uv_async_t *handle) {
    script_executor_t *exec = (script_executor_t *)handle->data;
    if (!exec || !exec->running) return;

    while (1) {
        script_task_t task;
        bool has_task = false;
        uv_mutex_lock(&exec->lock);
        if (exec->head != exec->tail) {
            task = exec->queue[exec->tail];
            exec->tail = (exec->tail + 1) % MAX_QUEUE_SIZE;
            has_task = true;
        }
        uv_mutex_unlock(&exec->lock);
        if (!has_task) break;

        script_stat_t *stat = _get_script_stat(exec, task.script_id);
        if (!_check_can_run(stat)) continue;

        char path[128];
        snprintf(path, 128, "config/scripts/%s.js", task.script_id);

        if (exec->engine) {
            // Only run if the file exists to avoid spamming errors
            FILE *f = fopen(path, "r");
            if (f) {
                fclose(f);
                int ret = js_engine_run_file(exec->engine, path);
                _report_result(stat, ret);
            }
        }
    }
}

static void _on_stop_async(uv_async_t *handle) {
    script_executor_t *exec = (script_executor_t *)handle->data;
    exec->running = false;
    uv_stop(&exec->compute_loop);
}

static void _on_handle_close(uv_handle_t *handle) {
    (void)handle;
}

static void _executor_entry(void *arg) {
    script_executor_t *exec = (script_executor_t *)arg;
    uv_loop_init(&exec->compute_loop);

    exec->engine = js_engine_create(NULL, 512 * 1024);

    uv_async_init(&exec->compute_loop, &exec->task_async, _process_tasks);
    exec->task_async.data = exec;

    uv_async_init(&exec->compute_loop, &exec->stop_async, _on_stop_async);
    exec->stop_async.data = exec;

    uv_run(&exec->compute_loop, UV_RUN_DEFAULT);

    if (exec->engine) {
        js_engine_destroy(exec->engine);
        exec->engine = NULL;
    }

    // Close async handles before loop close
    uv_close((uv_handle_t *)&exec->task_async, _on_handle_close);
    uv_close((uv_handle_t *)&exec->stop_async, _on_handle_close);

    // Drain loop to finish close callbacks
    uv_run(&exec->compute_loop, UV_RUN_DEFAULT);
    uv_loop_close(&exec->compute_loop);
}

script_executor_t *script_exec_create(uv_loop_t *main_loop) {
    (void)main_loop;
    script_executor_t *exec = calloc(1, sizeof(script_executor_t));
    if (!exec) return NULL;
    uv_mutex_init(&exec->lock);
    exec->running = true;
    if (uv_thread_create(&exec->tid, _executor_entry, exec) != 0) {
        uv_mutex_destroy(&exec->lock);
        free(exec);
        return NULL;
    }
    return exec;
}

int script_exec_post(script_executor_t *exec, const char *script_id, script_prio_t prio,
                     const PipeMsg *data) {
    if (!exec || !exec->running) return -1;
    uv_mutex_lock(&exec->lock);
    int next = (exec->head + 1) % MAX_QUEUE_SIZE;
    if (next == exec->tail) {
        uv_mutex_unlock(&exec->lock);
        return -1;
    }
    script_task_t *t = &exec->queue[exec->head];
    strncpy(t->script_id, script_id, 31);
    t->prio = prio;
    if (data) t->input = *data;
    exec->head = next;
    uv_mutex_unlock(&exec->lock);
    uv_async_send(&exec->task_async);
    return 0;
}

void script_exec_destroy(script_executor_t *exec) {
    if (!exec) return;
    exec->running = false;
    uv_async_send(&exec->stop_async);
    uv_thread_join(&exec->tid);
    uv_mutex_destroy(&exec->lock);
    free(exec);
}
