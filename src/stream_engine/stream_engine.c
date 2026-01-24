/**
 * @file stream_engine.c
 * @brief 流处理引擎核心实现 (契约驱动与多源触发增强版)
 */

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine_priv.h"

// --- Stage Factory Declarations ---
extern pipeline_stage_t *stage_filter_create(uint32_t tag_id);
extern pipeline_stage_t *stage_filter_range_create(double min, double max);
extern pipeline_stage_t *stage_transform_create(double alpha, double beta);
extern pipeline_stage_t *stage_ema_create(double alpha);
extern pipeline_stage_t *stage_clamp_create(double min, double max);
extern pipeline_stage_t *stage_peak_shaving_create(double threshold_w, uint32_t ctrl_tag);
extern pipeline_stage_t *stage_accumulate_create(void);
extern pipeline_stage_t *stage_periodic_freeze_create(int kind);
extern pipeline_stage_t *stage_time_guard_create(void);
extern pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table);
extern pipeline_stage_t *stage_js_rule_create(js_engine_t *engine, const char *script_path,
                                              bool is_async);
extern pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits);
extern pipeline_stage_t *stage_reflex_logic_create(struct bus_orchestrator_s *orch, uint32_t bus_id,
                                                   const char *proto, uint32_t target_tag,
                                                   double threshold, bool greater_than,
                                                   double action_val);
extern pipeline_stage_t *stage_moving_average_create(uint32_t window_size);
extern pipeline_stage_t *stage_rbe_create(double threshold_pct);
extern pipeline_stage_t *stage_energy_integrator_create(uint32_t state_tag_id);
extern pipeline_stage_t *stage_dsp_rms_create(uint32_t output_tag_id);

typedef struct {
    uint64_t last_seq;
} stream_sub_ctx_t;

#define DEFAULT_CIRCUIT_BREAKER_US 50000

static void _pipeline_process(pipeline_t *p, pipe_msg_t *msg) {
    // [Trace] 关键执行节点日志
    printf("\033[94m[STREAM] ⚙️  Processing Pipeline '%s' (Trigger: %u Val:%.2f)\033[0m\n",
           p->id, msg->tag_id, msg->payload.val.f);

    p->state = PIPE_STATE_RUNNING;
    pipeline_stage_t *s = p->head;
    while (s) {
        if (s->health != STAGE_HEALTH_GOOD) {
            s = s->next;
            continue;
        }

        uint64_t start = uv_hrtime();
        stage_ret_t ret = STAGE_OK;
        if (s->process) {
            ret = s->process(s, msg);
        }
        uint64_t elapsed_us = (uv_hrtime() - start) / 1000;

        s->last_proc_us = elapsed_us;
        s->total_events++;
        if (s->avg_proc_us == 0) {
            s->avg_proc_us = elapsed_us;
        } else {
            s->avg_proc_us = (s->avg_proc_us * 9 + elapsed_us) / 10;
        }

        if (s->circuit_breaker_us > 0 && elapsed_us > s->circuit_breaker_us) {
            s->health = STAGE_HEALTH_TIMEOUT;
            LOG_ERROR(LOG_CAT_SCHED,
                      "Stage '%s' in Pipeline '%s' CIRCUIT BROKEN! (Elapsed: %lu us)", s->name,
                      p->id, elapsed_us);
        }

        if (ret == STAGE_ASYNC) {
            p->state = PIPE_STATE_SUSPENDED;
            p->resume_point = s;
            p->current_event = *msg;
            break;
        }
        if (ret == STAGE_DROP) break;
        if (ret == STAGE_ERROR) {
            s->total_errors++;
            break;
        }
        s = s->next;
    }
    if (p->state == PIPE_STATE_RUNNING) {
        p->state = PIPE_STATE_IDLE;
    }
}

static bool _is_pipeline_triggered(pipeline_t *p, uint32_t tag_id) {
    if (p->trigger_tag == 0 || p->trigger_tag == tag_id) return true;
    for (uint32_t i = 0; i < p->extra_trigger_count; i++) {
        if (p->extra_triggers[i] == tag_id) return true;
    }
    return false;
}

static void _on_data_async(uv_async_t *handle) {
    stream_engine_t *engine = handle->data;
    datahub_t *hub = engine->hub;
    pipe_msg_t msg;

    // [Trace] 确定通知是否送达
    printf("\033[34m[STREAM-ASYNC] Wakeup received. Seq Cursor: %lu\033[0m\n",
           engine->bus_read_seq);

    uv_mutex_lock(&engine->pipe_mutex);
    int read_count = 0;
    while (shm_ring_read(&hub->ring, &engine->bus_read_seq, &msg) == 0) {
        read_count++;
        printf("[STREAM-TRACE] Read Tag %u from Ring.\n", msg.tag_id);
        struct uv__queue *q;
        uv__queue_foreach(q, &engine->pipelines) {
            pipeline_t *p = uv__queue_data(q, pipeline_t, node);
            if (_is_pipeline_triggered(p, msg.tag_id)) {
                if (p->state == PIPE_STATE_SUSPENDED) {
                    // ... 队列处理 ...
                } else {
                    _pipeline_process(p, &msg);
                }
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

static void _on_stop_async(uv_async_t *handle) {
    stream_engine_t *engine = (stream_engine_t *)handle->data;
    if (engine) uv_stop(engine->loop);
}

static void _on_compute_loop_handle_close(uv_handle_t *handle) {
    (void)handle;
}

static void _stream_engine_thread_entry(void *arg) {
    stream_engine_t *engine = (stream_engine_t *)arg;
    uv_loop_t *compute_loop = malloc(sizeof(uv_loop_t));
    if (!compute_loop) {
        engine->thread_started = false;
        uv_sem_post(&engine->start_sem);
        return;
    }
    uv_loop_init(compute_loop);
    engine->loop = compute_loop;
    uv_async_init(compute_loop, &engine->notify_handle, _on_data_async);
    engine->notify_handle.data = engine;
    engine->notify_inited = true;
    uv_async_init(compute_loop, &engine->stop_async, _on_stop_async);
    engine->stop_async.data = engine;
    engine->thread_started = true;
    uv_sem_post(&engine->start_sem);
    uv_run(compute_loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t *)&engine->notify_handle, _on_compute_loop_handle_close);
    uv_close((uv_handle_t *)&engine->stop_async, _on_compute_loop_handle_close);
    uv_run(compute_loop, UV_RUN_NOWAIT);
    uv_loop_close(compute_loop);
    free(compute_loop);
    engine->loop = NULL;
}

extern pipeline_stage_t *stage_energy_integrator_create(uint32_t state_tag_id);

static pipeline_stage_t *_create_stage_by_type(const edge_stage_def_t *def,
                                               pipeline_loader_ctx_t *ctx) {
    if (strcmp(def->type, "energy_integrator") == 0)
        return stage_energy_integrator_create((uint32_t)def->params.u1);
    if (strcmp(def->type, "dsp_rms") == 0) return stage_dsp_rms_create((uint32_t)def->params.u1);
    if (strcmp(def->type, "FILTER") == 0) return stage_filter_create((uint32_t)def->params.u1);
    if (strcmp(def->type, "FILTER_RANGE") == 0)
        return stage_filter_range_create(def->params.f1, def->params.f2);
    if (strcmp(def->type, "TRANSFORM") == 0)
        return stage_transform_create(def->params.f1, def->params.f2);
    if (strcmp(def->type, "EMA") == 0) return stage_ema_create(def->params.f1);
    if (strcmp(def->type, "CLAMP") == 0) return stage_clamp_create(def->params.f1, def->params.f2);
    if (strcmp(def->type, "PEAK_SHAVE") == 0)
        return stage_peak_shaving_create(def->params.f1, (uint32_t)def->params.u1);
    if (strcmp(def->type, "ACCUMULATE") == 0) return stage_accumulate_create();
    if (strcmp(def->type, "FREEZE") == 0) return stage_periodic_freeze_create((int)def->params.u1);
    if (strcmp(def->type, "TIME_GUARD") == 0) return stage_time_guard_create();
    if (strcmp(def->type, "SQLITE_SINK") == 0 && ctx->db)
        return stage_sqlite_sink_create(ctx->db, def->params.s1);
    if (strcmp(def->type, "JS_RULE") == 0 && ctx->js_engine)
        return stage_js_rule_create(ctx->js_engine, def->params.s1, false);
    if (strcmp(def->type, "REFLEX") == 0 && ctx->orch)
        return stage_reflex_logic_create(ctx->orch, 0, def->params.s1, (uint32_t)def->params.u1,
                                         def->params.f1, def->params.f2 > 0.5, def->params.f3);
    if (strcmp(def->type, "MOVING_AVERAGE") == 0)
        return stage_moving_average_create((uint32_t)def->params.u1);
    if (strcmp(def->type, "RBE") == 0) return stage_rbe_create(def->params.f1);
    return NULL;
}

int stream_engine_load_static_metadata(stream_engine_t *engine, const edge_pipeline_def_t *metadata,
                                       int count, pipeline_loader_ctx_t *ctx) {
    if (!engine || !metadata) return -1;
    for (int i = 0; i < count; i++) {
        const edge_pipeline_def_t *p_def = &metadata[i];
        pipeline_t *p = stream_engine_add_pipeline(engine, p_def->id, p_def->trigger_tag);

        // [Omnis] 加载额外触发点位
        if (p_def->extra_trigger_count > 0) {
            p->extra_triggers = calloc(p_def->extra_trigger_count, sizeof(uint32_t));
            memcpy(p->extra_triggers, p_def->extra_triggers,
                   p_def->extra_trigger_count * sizeof(uint32_t));
            p->extra_trigger_count = p_def->extra_trigger_count;
        }

        for (uint32_t j = 0; j < p_def->stage_count; j++) {
            const edge_stage_def_t *s_def = &p_def->stages[j];
            pipeline_stage_t *stage = _create_stage_by_type(s_def, ctx);
            if (stage) {
                pipeline_add_stage(p, stage->name, stage->process, stage->priv_state,
                                   stage->cleanup_state, stage->priv_cfg, stage->cleanup_cfg);
                free(stage);
            }
        }
    }
    return 0;
}

pipeline_stage_t *pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc,
                                     void *priv_state, stage_cleanup_fn cleanup_state,
                                     void *priv_cfg, stage_cleanup_fn cleanup_cfg) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, name, 31);
    s->loop = p->engine->loop;
    s->pipeline = p;
    s->process = proc;
    s->priv_state = priv_state;
    s->cleanup_state = cleanup_state;
    s->priv_cfg = priv_cfg;
    s->cleanup_cfg = cleanup_cfg;
    if (!p->head)
        p->head = p->tail = s;
    else {
        p->tail->next = s;
        p->tail = s;
    }
    return s;
}

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    if (time_src) engine->time_src = *time_src;

    // [Fix] 跳过历史数据：初始化 Seq 为当前 DataHub 的最新序号
    if (hub) {
        engine->bus_read_seq =
            (uint64_t)atomic_load_explicit(&hub->ring.hdr->seq, memory_order_acquire);
        printf("[STREAM-INIT] Synced to Ring Seq: %lu\n", engine->bus_read_seq);
    }

    uv_mutex_init(&engine->pipe_mutex);
    uv__queue_init(&engine->pipelines);
    uv_sem_init(&engine->start_sem, 0);
    if (uv_thread_create(&engine->thread, _stream_engine_thread_entry, engine) != 0) {
        uv_mutex_destroy(&engine->pipe_mutex);
        uv_sem_destroy(&engine->start_sem);
        free(engine);
        return NULL;
    }
    uv_sem_wait(&engine->start_sem);
    uv_sem_destroy(&engine->start_sem);
    if (!engine->thread_started) {
        uv_thread_join(&engine->thread);
        uv_mutex_destroy(&engine->pipe_mutex);
        free(engine);
        return NULL;
    }
    datahub_add_listener(hub, &engine->notify_handle);
    return engine;
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;
    if (engine->thread_started) {
        datahub_remove_listener(engine->hub, &engine->notify_handle);
        uv_async_send(&engine->stop_async);
        uv_thread_join(&engine->thread);
    }
    uv_mutex_lock(&engine->pipe_mutex);
    while (!uv__queue_empty(&engine->pipelines)) {
        struct uv__queue *q = uv__queue_head(&engine->pipelines);
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        uv__queue_remove(q);
        pipeline_stage_t *s = p->head;
        while (s) {
            pipeline_stage_t *next_s = s->next;
            if (s->cleanup_state) s->cleanup_state(s->priv_state);
            if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
            free(s);
            s = next_s;
        }
        if (p->extra_triggers) free(p->extra_triggers);  // [Omnis] 释放扩展触发器
        free(p);
    }
    uv_mutex_unlock(&engine->pipe_mutex);
    uv_mutex_destroy(&engine->pipe_mutex);
    free(engine);
}

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag) {
    pipeline_t *p = calloc(1, sizeof(pipeline_t));
    strncpy(p->id, id, 63);
    p->trigger_tag = trigger_tag;
    p->engine = engine;
    uv_mutex_lock(&engine->pipe_mutex);
    uv__queue_insert_tail(&engine->pipelines, &p->node);
    engine->pipe_count++;
    uv_mutex_unlock(&engine->pipe_mutex);
    return p;
}

void stream_engine_resume(pipeline_t *p) {
    if (!p || p->state != PIPE_STATE_SUSPENDED) return;
    int depth = (p->q_head - p->q_tail + MAX_PENDING_QUEUE) % MAX_PENDING_QUEUE;
    if (depth > 0) {
        pipe_msg_t q_msg;
        pipe_msg_init_scalar(&q_msg, 11005, (double)depth, uv_hrtime());
        datahub_publish(p->engine->hub, &q_msg);
    }
    p->state = PIPE_STATE_RUNNING;
    pipeline_stage_t *s = p->resume_point ? p->resume_point->next : NULL;
    while (s) {
        if (s->health != STAGE_HEALTH_GOOD) {
            s = s->next;
            continue;
        }
        stage_ret_t ret = s->process(s, &p->current_event);
        if (ret == STAGE_ASYNC) {
            p->state = PIPE_STATE_SUSPENDED;
            p->resume_point = s;
            return;
        }
        if (ret == STAGE_DROP || ret == STAGE_ERROR) break;
        s = s->next;
    }
    p->state = PIPE_STATE_IDLE;
    p->resume_point = NULL;
    while (p->q_head != p->q_tail) {
        pipe_msg_t queued_msg = p->queue[p->q_tail];
        p->q_tail = (p->q_tail + 1) % MAX_PENDING_QUEUE;
        _pipeline_process(p, &queued_msg);
        if (p->state == PIPE_STATE_SUSPENDED) break;
    }
}

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg) {
    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        bool matched = _is_pipeline_triggered(p, msg->tag_id);
        if (matched) {
            if (msg->type == PIPEMSG_TYPE_REF) {
                printf("\033[32m[STREAM-MATCH] Pipeline '%s' triggered by TYPE_REF Tag %u\033[0m\n",
                       p->id, msg->tag_id);
            }
            _pipeline_process(p, msg);
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

stage_health_t pipeline_stage_get_health(pipeline_stage_t *stage) {
    return stage ? stage->health : STAGE_HEALTH_GOOD;
}

uv_async_t *stream_engine_get_notify_handle(stream_engine_t *engine) {
    return engine ? &engine->notify_handle : NULL;
}
