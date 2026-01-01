#include <cJSON/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edge_log.h"
#include "sched_time.h"
#include "stream_engine_priv.h"

static uint64_t default_mono_ms(void) {
    return uv_hrtime() / 1000000;
}

static uint64_t default_cal_ms(void) {
    return (uint64_t)time(NULL) * 1000;
}

// Forward declarations
static void on_time_guard(uv_timer_t *handle);
static void stream_worker(void *arg);

// --- Pipeline Logic ---

static void pipeline_execute(pipeline_t *p, pipe_msg_t *ev);

static void process_next_in_queue(pipeline_t *p) {
    if (p->q_head == p->q_tail) {
        p->state = PIPE_IDLE;
        return;
    }
    p->state = PIPE_RUNNING;
    p->current_event = p->queue[p->q_tail];
    p->q_tail = (p->q_tail + 1) % MAX_PENDING_QUEUE;
    pipeline_execute(p, &p->current_event);
}

static void pipeline_execute(pipeline_t *p, pipe_msg_t *ev) {
    pipeline_stage_t *stage = p->resume_point ? p->resume_point : p->head;
    p->resume_point = NULL;

    while (stage) {
        stage_ret_t ret = stage->process(stage, ev);

        if (ret == STAGE_DROP) {
            process_next_in_queue(p);
            return;
        }
        if (ret == STAGE_ASYNC) {
            p->state = PIPE_SUSPENDED;
            p->resume_point = stage->next;
            return;  // Yield
        }
        // STAGE_OK -> Next
        stage = stage->next;
    }
    // Done
    process_next_in_queue(p);
}

void pipeline_resume(pipeline_t *p) {
    if (p->state == PIPE_SUSPENDED) {
        p->state = PIPE_RUNNING;
        pipeline_execute(p, &p->current_event);
    }
}

int stream_engine_push_event(stream_engine_t *engine, pipe_msg_t *ev) {
    uv_mutex_lock(&engine->pipe_mutex);
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        if (p->trigger_tag == ev->tag_id ||
            (ev->type == PIPEMSG_TYPE_EVENT &&
             ev->payload.event.event_id == SYS_EVENT_TIME_CHANGED)) {
            // 入队
            int next = (p->q_head + 1) % MAX_PENDING_QUEUE;
            if (next != p->q_tail) {
                p->queue[p->q_head] = *ev;
                p->q_head = next;
                if (p->state == PIPE_IDLE) process_next_in_queue(p);
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
    return 0;
}

// --- DataHub Bridge ---

static void stream_engine_pump_drain(stream_engine_t *engine) {
    PipeMsg msg;
    while (shm_ring_read(&engine->hub->ring, &engine->bus_read_seq, &msg) == 0) {
        pipe_msg_t ev = msg;
        stream_engine_push_event(engine, &ev);
    }
}

static void on_data_notify(uv_async_t *h) {
    stream_engine_pump_drain((stream_engine_t *)h->data);
}

static void datahub_bridge_cb(datahub_t *hub, void *user_data) {
    (void)hub;
    stream_engine_t *engine = (stream_engine_t *)user_data;
    uv_async_send(&engine->notify_handle);
}

// --- Time Guard ---

static void on_time_guard(uv_timer_t *handle) {
    stream_engine_t *engine = (stream_engine_t *)handle->data;
    uint64_t cal_now = engine->time_src.get_calendar_ms();
    uint64_t current_min = cal_now / 60000;

    if (current_min > engine->last_calendar_min) {
        pipe_msg_t tick = {0};
        tick.ts = cal_now;
        tick.type = PIPEMSG_TYPE_EVENT;
        tick.payload.event.event_id = SYS_EVENT_USER_CUSTOM;
        tick.payload.event.param = 1;  // Minute Tick

        stream_engine_push_event(engine, &tick);
        engine->last_calendar_min = current_min;
    }
}

// --- Lifecycle ---

static void stream_worker(void *arg) {
    stream_engine_t *engine = (stream_engine_t *)arg;
    uv_run(engine->loop, UV_RUN_DEFAULT);
}

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    engine->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(engine->loop);

    if (time_src) {
        engine->time_src = *time_src;
    } else {
        engine->time_src.get_monotonic_ms = default_mono_ms;
        engine->time_src.get_calendar_ms = default_cal_ms;
    }

    uv_mutex_init(&engine->pipe_mutex);
    uv_async_init(engine->loop, &engine->notify_handle, on_data_notify);
    engine->notify_handle.data = engine;
    engine->notify_inited = true;

    uv_timer_init(engine->loop, &engine->time_guard);
    engine->time_guard.data = engine;
    uv_timer_start(&engine->time_guard, on_time_guard, 100, 100);
    engine->timer_inited = true;

    if (hub) {
        hub->on_data = datahub_bridge_cb;
        hub->user_data = engine;
    }

    if (uv_thread_create(&engine->thread, stream_worker, engine) == 0) {
        engine->thread_started = true;
    }

    LOG_INFO(LOG_CAT_CORE, "Stream Engine started (Threaded Mode)");
    return engine;
}

// ... (Rest of functions: add_pipeline, pipeline_add_stage, clear_pipelines, dump_json, destroy)
// 保持不变，只需追加到文件末尾

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag) {
    uv_mutex_lock(&engine->pipe_mutex);
    if (engine->pipe_count >= MAX_PIPELINES) {
        uv_mutex_unlock(&engine->pipe_mutex);
        return NULL;
    }
    pipeline_t *p = calloc(1, sizeof(pipeline_t));
    strncpy(p->id, id, 63);
    p->trigger_tag = trigger_tag;
    engine->pipelines[engine->pipe_count++] = p;
    uv_mutex_unlock(&engine->pipe_mutex);
    return p;
}

void pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc, void *priv_state,
                        stage_cleanup_fn cleanup_state, void *priv_cfg,
                        stage_cleanup_fn cleanup_cfg) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, name, 31);
    s->process = proc;
    s->priv_state = priv_state;
    s->cleanup_state = cleanup_state;
    s->priv_cfg = priv_cfg;
    s->cleanup_cfg = cleanup_cfg;

    if (!p->head) {
        p->head = s;
        p->tail = s;
    } else {
        p->tail->next = s;
        p->tail = s;
    }
}

static void stream_engine_clear_pipelines(stream_engine_t *engine) {
    uv_mutex_lock(&engine->pipe_mutex);
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        pipeline_stage_t *curr = p->head;
        while (curr) {
            pipeline_stage_t *next = curr->next;
            if (curr->cleanup_state) curr->cleanup_state(curr->priv_state);
            if (curr->cleanup_cfg) curr->cleanup_cfg(curr->priv_cfg);
            free(curr);
            curr = next;
        }
        free(p);
    }
    engine->pipe_count = 0;
    uv_mutex_unlock(&engine->pipe_mutex);
}

char *stream_engine_dump_json(stream_engine_t *engine) {
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(root, "pipelines");

    uv_mutex_lock(&engine->pipe_mutex);
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        cJSON *pipe_json = cJSON_CreateObject();
        cJSON_AddStringToObject(pipe_json, "id", p->id);
        cJSON_AddNumberToObject(pipe_json, "trigger_tag", p->trigger_tag);

        const char *state_str = "IDLE";
        if (p->state == PIPE_RUNNING)
            state_str = "RUNNING";
        else if (p->state == PIPE_SUSPENDED)
            state_str = "SUSPENDED";
        cJSON_AddStringToObject(pipe_json, "state", state_str);

        cJSON *stages = cJSON_AddArrayToObject(pipe_json, "stages");
        pipeline_stage_t *curr = p->head;
        while (curr) {
            cJSON_AddItemToArray(stages, cJSON_CreateString(curr->name));
            curr = curr->next;
        }
        cJSON_AddItemToArray(arr, pipe_json);
    }
    uv_mutex_unlock(&engine->pipe_mutex);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;

    if (engine->thread_started) {
        uv_stop(engine->loop);
        uv_async_send(&engine->notify_handle);
        uv_thread_join(&engine->thread);
    }

    if (engine->notify_inited) uv_close((uv_handle_t *)&engine->notify_handle, NULL);
    if (engine->timer_inited) uv_close((uv_handle_t *)&engine->time_guard, NULL);

    uv_run(engine->loop, UV_RUN_NOWAIT);
    uv_loop_close(engine->loop);
    free(engine->loop);

    stream_engine_clear_pipelines(engine);
    uv_mutex_destroy(&engine->pipe_mutex);
    free(engine);
}
