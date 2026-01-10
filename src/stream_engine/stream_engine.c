/**
 * @file stream_engine.c
 * @brief 流处理引擎核心实现 (契约驱动与 Queue 结构对齐版)
 */

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "stream_engine_priv.h"

#define DEFAULT_CIRCUIT_BREAKER_US 50000

static void _pipeline_process(pipeline_t *p, pipe_msg_t *msg) {
    fprintf(stderr, "[DEBUG][StreamEngine] Processing Pipeline: %s\n", p->id);
    pipeline_stage_t *s = p->head;
    while (s) {
        fprintf(stderr, "[DEBUG][StreamEngine] Executing Stage: %s\n", s->name);
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

        if (s->circuit_breaker_us > 0 && elapsed_us > s->circuit_breaker_us) {
            s->health = STAGE_HEALTH_TIMEOUT;
            LOG_ERROR(LOG_CAT_SCHED,
                      "Stage '%s' in Pipeline '%s' CIRCUIT BROKEN! (Elapsed: %lu us)", s->name,
                      p->id, elapsed_us);
        }

        if (ret == STAGE_DROP) break;
        if (ret == STAGE_ERROR) {
            s->total_errors++;
            break;
        }
        s = s->next;
    }
}

static void _on_data_async(uv_async_t *handle) {
    stream_engine_t *engine = handle->data;
    datahub_t *hub = engine->hub;
    pipe_msg_t msg;

    fprintf(stderr, "[DEBUG][StreamEngine] Async signal received!\n");
    uv_mutex_lock(&engine->pipe_mutex);
    while (shm_ring_read(&hub->ring, &engine->bus_read_seq, &msg) == 0) {
        fprintf(stderr, "[DEBUG][StreamEngine] Read Tag %u from ring.\n", msg.tag_id);
        struct uv__queue *q;
        uv__queue_foreach(q, &engine->pipelines) {
            pipeline_t *p = uv__queue_data(q, pipeline_t, node);
            if (p->trigger_tag == msg.tag_id || p->trigger_tag == 0) {
                fprintf(stderr, "[DEBUG][StreamEngine] Matching Pipeline found: %s\n", p->id);
                _pipeline_process(p, &msg);
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

static pipeline_stage_t *_create_stage_by_type(const edge_stage_def_t *def,
                                               pipeline_loader_ctx_t *ctx) {
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
        return stage_js_rule_create(ctx->js_engine, def->params.s1);
    if (strcmp(def->type, "REFLEX") == 0 && ctx->orch) {
        return stage_reflex_logic_create(ctx->orch, 0, def->params.s1, (uint32_t)def->params.u1,
                                         def->params.f1, def->params.f2 > 0.5);
    }
    if (strcmp(def->type, "MOVING_AVERAGE") == 0)
        return stage_moving_average_create((uint32_t)def->params.u1);
    if (strcmp(def->type, "RBE") == 0) return stage_rbe_create(def->params.f1);

    return NULL;
}

int stream_engine_load_static_metadata(stream_engine_t *engine, const edge_pipeline_def_t *metadata,
                                       int count, pipeline_loader_ctx_t *ctx) {
    if (!engine || !metadata) return -1;
    LOG_INFO(LOG_CAT_SCHED, "StreamEngine: Assembling %d pipelines from static metadata.", count);

    for (int i = 0; i < count; i++) {
        const edge_pipeline_def_t *p_def = &metadata[i];
        pipeline_t *p = stream_engine_add_pipeline(engine, p_def->id, p_def->trigger_tag);

        for (uint32_t j = 0; j < p_def->stage_count; j++) {
            const edge_stage_def_t *s_def = &p_def->stages[j];
            pipeline_stage_t *stage = _create_stage_by_type(s_def, ctx);
            if (stage) {
                if (!p->head)
                    p->head = p->tail = stage;
                else {
                    p->tail->next = stage;
                    p->tail = stage;
                }
            }
        }
    }
    return 0;
}

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    engine->loop = hub->loop;
    if (time_src) engine->time_src = *time_src;

    uv__queue_init(&engine->pipelines);
    uv_mutex_init(&engine->pipe_mutex);
    uv_async_init(engine->loop, &engine->notify_handle, _on_data_async);
    engine->notify_handle.data = engine;
    engine->notify_inited = true;

    datahub_add_listener(hub, &engine->notify_handle);
    return engine;
}

stream_engine_t *stream_engine_create_ex(datahub_t *hub, uv_loop_t *loop,
                                         engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    engine->loop = loop;
    if (time_src) engine->time_src = *time_src;

    uv__queue_init(&engine->pipelines);
    uv_mutex_init(&engine->pipe_mutex);
    uv_async_init(loop, &engine->notify_handle, _on_data_async);
    engine->notify_handle.data = engine;
    engine->notify_inited = true;

    datahub_add_listener(hub, &engine->notify_handle);
    return engine;
}

static void _on_engine_close(uv_handle_t *handle) {
    stream_engine_t *engine = handle->data;
    uv_mutex_destroy(&engine->pipe_mutex);
    free(engine);
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;

    datahub_remove_listener(engine->hub, &engine->notify_handle);

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
        free(p);
    }
    uv_mutex_unlock(&engine->pipe_mutex);

    if (engine->notify_inited && !uv_is_closing((uv_handle_t *)&engine->notify_handle)) {
        uv_close((uv_handle_t *)&engine->notify_handle, _on_engine_close);
    } else {
        uv_mutex_destroy(&engine->pipe_mutex);
        free(engine);
    }
}

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag) {
    pipeline_t *p = calloc(1, sizeof(pipeline_t));
    strncpy(p->id, id, 63);
    p->trigger_tag = trigger_tag;

    uv_mutex_lock(&engine->pipe_mutex);
    uv__queue_insert_tail(&engine->pipelines, &p->node);
    engine->pipe_count++;
    uv_mutex_unlock(&engine->pipe_mutex);

    return p;
}

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg) {
    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        if (p->trigger_tag == msg->tag_id || p->trigger_tag == 0) {
            _pipeline_process(p, msg);
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

stage_health_t pipeline_stage_get_health(pipeline_stage_t *stage) {
    return stage ? stage->health : STAGE_HEALTH_GOOD;
}