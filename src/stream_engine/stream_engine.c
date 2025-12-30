#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "stream_engine_priv.h"

// --- 时间源默认实现 ---
static uint64_t default_mono_ms(void) {
    return uv_hrtime() / 1000000;
}
static uint64_t default_cal_ms(void) {
    uv_timeval64_t tv;
    uv_gettimeofday(&tv);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

// --- FSM 调度核心 ---
static void pipeline_execute(pipeline_t *p, pipe_event_t *ev);

static void process_next_in_queue(pipeline_t *p) {
    if (p->q_head != p->q_tail && p->state == PIPE_IDLE) {
        pipe_event_t next_ev = p->queue[p->q_tail];
        p->q_tail = (p->q_tail + 1) % MAX_PENDING_QUEUE;
        pipeline_execute(p, &next_ev);
    }
}

static void pipeline_execute(pipeline_t *p, pipe_event_t *ev) {
    if (!p) return;
    pipeline_stage_t *curr = p->resume_point ? p->resume_point : p->head;
    if (!p->resume_point) p->current_event = *ev;
    p->resume_point = NULL;
    p->state = PIPE_RUNNING;

    while (curr) {
        stage_ret_t ret = curr->process(curr, ev);
        if (ret == STAGE_ASYNC) {
            p->state = PIPE_SUSPENDED;
            p->resume_point = curr->next;
            return;
        }
        if (ret == STAGE_DROP || ret == STAGE_ERROR) break;
        curr = curr->next;
    }
    p->state = PIPE_IDLE;
    process_next_in_queue(p);
}

void pipeline_resume(pipeline_t *p) {
    if (!p || p->state != PIPE_SUSPENDED) return;
    p->state = PIPE_IDLE;
    pipeline_execute(p, &p->current_event);
}

int stream_engine_push_event(stream_engine_t *engine, pipe_event_t *ev) {
    if (!engine || !ev) return -1;
    uv_mutex_lock(&engine->pipe_mutex);
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        if (p->trigger_tag == 0 || p->trigger_tag == ev->tag_id || ev->type == EVT_TIME_PULSE) {
            if (p->state == PIPE_SUSPENDED) {
                int next = (p->q_head + 1) % MAX_PENDING_QUEUE;
                if (next != p->q_tail) {
                    p->queue[p->q_head] = *ev;
                    p->q_head = next;
                }
            } else {
                pipeline_execute(p, ev);
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
    return 0;
}

// --- DataPump 影子流 ---
void stream_engine_pump_drain(stream_engine_t *engine) {
    if (!engine || !engine->hub) return;
    PipeMsg msg;
    uv_mutex_lock(&engine->pipe_mutex);
    while (shm_ring_read(&engine->hub->ring, &engine->bus_read_seq, &msg) == 0) {
        pipe_event_t ev = {.type = EVT_DATA_POINT,
                           .tag_id = msg.tag_id,
                           .payload.val = msg.payload.val.f,
                           .calendar_ms = msg.ts};
        uv_mutex_unlock(&engine->pipe_mutex);
        stream_engine_push_event(engine, &ev);
        uv_mutex_lock(&engine->pipe_mutex);
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

static void on_data_notify(uv_async_t *h) {
    stream_engine_pump_drain((stream_engine_t *)h->data);
}
static void datahub_bridge_cb(datahub_t *hub, void *user_data) {
    (void)hub;
    uv_async_send((uv_async_t *)user_data);
}

static void on_time_guard(uv_timer_t *h) {
    stream_engine_t *engine = (stream_engine_t *)h->data;
    uint64_t cal_now = engine->time_src.get_calendar_ms();
    uint64_t current_min = cal_now / 60000;
    if (current_min != engine->last_calendar_min) {
        engine->last_calendar_min = current_min;
        pipe_event_t tick = {
            .type = EVT_TIME_PULSE, .calendar_ms = cal_now, .payload.tick_kind = 1};
        stream_engine_push_event(engine, &tick);
    }
}

// --- Lifecycle ---

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    if (!hub) return NULL;
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->loop = uv_loop_new();
    engine->hub = hub;
    if (time_src)
        engine->time_src = *time_src;
    else {
        engine->time_src.get_monotonic_ms = default_mono_ms;
        engine->time_src.get_calendar_ms = default_cal_ms;
    }

    uv_mutex_init(&engine->pipe_mutex);
    uv_async_init(engine->loop, &engine->notify_handle, on_data_notify);
    engine->notify_handle.data = engine;
    engine->notify_inited = true;
    uv_timer_init(engine->loop, &engine->time_guard);
    engine->time_guard.data = engine;
    engine->timer_inited = true;

    hub->user_data = &engine->notify_handle;
    hub->on_data = datahub_bridge_cb;
    return engine;
}

static void thread_wrapper(void *arg) {
    uv_run((uv_loop_t *)arg, UV_RUN_DEFAULT);
}
int stream_engine_start(stream_engine_t *engine) {
    if (!engine) return -1;
    uv_timer_start(&engine->time_guard, on_time_guard, 1000, 1000);
    engine->thread_started = true;
    return uv_thread_create(&engine->thread, thread_wrapper, engine->loop);
}

void pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc, void *init_state,
                        void *init_cfg, void (*cleanup)(void *)) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, name, 31);
    s->process = proc;
    s->priv_state = init_state;
    s->priv_cfg = init_cfg;
    s->cleanup = cleanup;
    if (!p->head)
        p->head = p->tail = s;
    else {
        p->tail->next = s;
        p->tail = s;
    }
}

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag) {
    uv_mutex_lock(&engine->pipe_mutex);
    if (engine->pipe_count >= 32) {
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

static void on_walk_close(uv_handle_t *h, void *arg) {
    (void)arg;
    if (!uv_is_closing(h)) uv_close(h, NULL);
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;
    if (engine->hub) {
        engine->hub->on_data = NULL;
    }
    if (engine->thread_started) {
        uv_stop(engine->loop);
        uv_thread_join(&engine->thread);
    }

    if (engine->loop) {
        uv_walk(engine->loop, on_walk_close, NULL);
        while (uv_run(engine->loop, UV_RUN_ONCE));
        uv_loop_delete(engine->loop);
    }
    uv_mutex_destroy(&engine->pipe_mutex);
    for (int i = 0; i < engine->pipe_count; i++) {
        pipeline_t *p = engine->pipelines[i];
        pipeline_stage_t *curr = p->head;
        while (curr) {
            pipeline_stage_t *next = curr->next;
            if (curr->cleanup) curr->cleanup(curr->priv_state);
            free(curr);
            curr = next;
        }
        free(p);
    }
    free(engine);
}
