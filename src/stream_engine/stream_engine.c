/**
 * @file stream_engine.c
 * @brief 流处理引擎核心实现 - 徹底实现拓扑图计算模型与状态检查点
 */

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "edge_log.h"
#include "stream_engine_priv.h"
#include "sqlite_service.h"
#include "wasm_mgr.h"

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

// --- 内部时间源默认实现 ---
static uint64_t _default_get_monotonic_ms(void) {
    return uv_hrtime() / 1000000;
}

static uint64_t _default_get_calendar_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

static void _pipeline_process(pipeline_t *p, pipe_msg_t *msg) {
    p->state = PIPE_STATE_RUNNING;
    pipeline_stage_t *s = p->head;
    while (s) {
        if (s->health != STAGE_HEALTH_GOOD) {
            s = s->next[0];
            continue;
        }

        uint64_t start = uv_hrtime();
        stage_ret_t ret = STAGE_OK;
        if (s->process) {
            ret = s->process(s, msg);
        } else {
            s = s->next[0];
            continue;
        }
        uint64_t elapsed_us = (uv_hrtime() - start) / 1000;

        s->last_proc_us = elapsed_us;
        s->total_events++;
        if (s->avg_proc_us == 0) {
            s->avg_proc_us = elapsed_us;
        } else {
            s->avg_proc_us = (s->avg_proc_us * 9 + elapsed_us) / 10;
        }

        if (ret == STAGE_ASYNC) {
            p->state = PIPE_STATE_SUSPENDED;
            p->resume_point = s;
            p->current_event = *msg;
            break;
        }
        
        if (ret == STAGE_DROP || ret == STAGE_ERROR) break;

        if (ret >= STAGE_BRANCH_0 && ret <= STAGE_BRANCH_3) {
            s = s->next[ret - STAGE_BRANCH_0];
        } else {
            s = s->next[0];
        }
    }
    if (p->state == PIPE_STATE_RUNNING) p->state = PIPE_STATE_IDLE;
}

static bool _is_pipeline_triggered(pipeline_t *p, uint32_t tag_id) {
    if (!p) return false;
    if (p->trigger_tag == 0 || p->trigger_tag == tag_id) return true;
    if (p->extra_triggers) {
        for (uint32_t i = 0; i < p->extra_trigger_count; i++) {
            if (p->extra_triggers[i] == tag_id) return true;
        }
    }
    return false;
}

static void _on_data_async(uv_async_t *handle) {
    stream_engine_t *engine = handle->data;
    datahub_t *hub = engine->hub;
    pipe_msg_t msg;

    uv_mutex_lock(&engine->pipe_mutex);
    int rc;
    while ((rc = shm_ring_read(&hub->ring, &engine->bus_read_seq, &msg)) >= 0) {
        if (rc > 0) {
            LOG_WARN(LOG_CAT_CORE, "StreamEngine: ShmRing dropped %d messages", rc);
        }
        struct uv__queue *q;
        uv__queue_foreach(q, &engine->pipelines) {
            pipeline_t *p = uv__queue_data(q, pipeline_t, node);
            if (_is_pipeline_triggered(p, msg.tag_id)) {
                pipe_msg_t result = msg;
                _pipeline_process(p, &result);
                // [Nexus] Automatically publish result back to DataHub if changed OR different tag
                if (p->state == PIPE_STATE_IDLE) {
                    if (result.tag_id != msg.tag_id || result.payload.val.f != msg.payload.val.f) {
                         LOG_INFO(LOG_CAT_CORE, "StreamEngine: Publishing result for Tag %u (Val: %.2f)", result.tag_id, result.payload.val.f);
                         datahub_publish(engine->hub, &result);
                    }
                }
            }
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

static void _on_stop_async(uv_async_t *handle) {
    uv_stop(handle->loop);
}

static void _on_handle_close(uv_handle_t *handle) {
    (void)handle;
}

static void _safe_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, _on_handle_close);
    }
}

static void _stream_engine_thread_entry(void *arg) {
    stream_engine_t *engine = (stream_engine_t *)arg;
    uv_loop_t *compute_loop = malloc(sizeof(uv_loop_t));
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

    uv_walk(compute_loop, _safe_close, NULL);
    while (uv_run(compute_loop, UV_RUN_ONCE))
        ;

    uv_loop_close(compute_loop);
    free(compute_loop);
    engine->loop = NULL;
    engine->notify_inited = false;
}

static pipeline_stage_t *_create_stage_by_type(stream_engine_t *engine, const edge_stage_def_t *def,
                                               pipeline_loader_ctx_t *ctx) {
    const edge_stage_factory_entry_t *reg = engine->registry;
    if (reg) {
        for (int i = 0; reg[i].type != NULL; i++) {
            if (strcasecmp(def->type, reg[i].type) == 0) {
                if (reg[i].factory) {
                    return reg[i].factory(def, ctx);
                }
            }
        }
    }
    LOG_ERROR(LOG_CAT_SCHED, "Unknown or disabled stage type: %s", def->type);
    return NULL;
}

int stream_engine_load_static_metadata(stream_engine_t *engine, const edge_pipeline_def_t *metadata,
                                       int count, pipeline_loader_ctx_t *ctx) {
    if (!engine || !metadata) return -1;
    for (int i = 0; i < count; i++) {
        const edge_pipeline_def_t *p_def = &metadata[i];
        pipeline_t *p = stream_engine_add_pipeline(engine, p_def->id, p_def->trigger_tag);

        if (p_def->extra_trigger_count > 0) {
            p->extra_triggers = calloc(p_def->extra_trigger_count, sizeof(uint32_t));
            memcpy(p->extra_triggers, p_def->extra_triggers,
                   p_def->extra_trigger_count * sizeof(uint32_t));
            p->extra_trigger_count = p_def->extra_trigger_count;
        }

        pipeline_stage_t **stages = calloc(p_def->stage_count, sizeof(pipeline_stage_t *));
        for (uint32_t j = 0; j < p_def->stage_count; j++) {
            stages[j] = _create_stage_by_type(engine, &p_def->stages[j], ctx);
            if (stages[j]) {
                stages[j]->pipeline = p;
                stages[j]->loop = engine->loop;
                if (!p->head) p->head = stages[j];
            }
        }

        for (uint32_t j = 0; j < p_def->stage_count; j++) {
            if (!stages[j]) continue;
            const edge_stage_def_t *s_def = &p_def->stages[j];
            for (int k = 0; k < 4; k++) {
                int next_idx = s_def->next_indices[k];
                if (next_idx >= 0 && (uint32_t)next_idx < p_def->stage_count) {
                    stages[j]->next[k] = stages[next_idx];
                } else if (k == 0 && j + 1 < p_def->stage_count) {
                    stages[j]->next[k] = stages[j + 1];
                }
            }
        }
        free(stages);
    }
    return 0;
}

pipeline_stage_t *pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc,
                                     void *priv_state, stage_cleanup_fn cleanup_state,
                                     void *priv_cfg, stage_cleanup_fn cleanup_cfg) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, name, 31);
    s->pipeline = p;
    s->process = proc;
    s->priv_state = priv_state;
    s->cleanup_state = cleanup_state;
    s->priv_cfg = priv_cfg;
    s->cleanup_cfg = cleanup_cfg;
    if (!p->head)
        p->head = s;
    else {
        pipeline_stage_t *last = p->head;
        while(last->next[0]) last = last->next[0];
        last->next[0] = s;
    }
    return s;
}

stream_engine_t *stream_engine_create_ex(datahub_t *hub, uv_loop_t *loop,
                                         struct bus_orchestrator_s *orch,
                                         const edge_stage_factory_entry_t *registry) {
    (void)loop;
    (void)orch;
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    engine->time_src.get_monotonic_ms = _default_get_monotonic_ms;
    engine->time_src.get_calendar_ms = _default_get_calendar_ms;
    engine->registry = registry;

    if (hub) {
        engine->bus_read_seq =
            (uint64_t)atomic_load_explicit(&hub->ring.hdr->seq, memory_order_acquire);
    }

    uv_mutex_init_recursive(&engine->pipe_mutex);
    uv__queue_init(&engine->pipelines);
    uv_sem_init(&engine->start_sem, 0);

#ifdef ENABLE_WASM
    wasm_mgr_init(hub);
#endif

    return engine;
}

stream_engine_t *stream_engine_create(datahub_t *hub, uv_loop_t *loop,
                                      struct bus_orchestrator_s *orch,
                                      const edge_stage_factory_entry_t *registry) {
    return stream_engine_create_ex(hub, loop, orch, registry);
}

void stream_engine_start(stream_engine_t *engine) {
    if (!engine || engine->thread_started) return;
    uv_thread_create(&engine->thread, _stream_engine_thread_entry, engine);
    uv_sem_wait(&engine->start_sem);
    if (engine->notify_inited) {
        datahub_add_listener(engine->hub, &engine->notify_handle);
    }
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;
    if (engine->thread_started) {
        if (engine->notify_inited) {
            datahub_remove_listener(engine->hub, &engine->notify_handle);
        }
        uv_async_send(&engine->stop_async);
        uv_thread_join(&engine->thread);
        engine->thread_started = false;
    }

    uv_mutex_lock(&engine->pipe_mutex);
    while (!uv__queue_empty(&engine->pipelines)) {
        struct uv__queue *q = uv__queue_head(&engine->pipelines);
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        uv__queue_remove(q);

        pipeline_stage_t *s = p->head;
        while (s) {
            pipeline_stage_t *next_s = s->next[0];
            // [Nexus] Avoid double free if stages are shared or chain is corrupted
            if (s->health == 99) break; // Use health=99 as "already freed" marker for this slice
            
            if (s->cleanup_state) s->cleanup_state(s->priv_state);
            if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
            s->health = 99;
            free(s);
            s = next_s;
        }
        if (p->extra_triggers) free(p->extra_triggers);
        free(p);
    }
    uv_mutex_unlock(&engine->pipe_mutex);

    uv_mutex_destroy(&engine->pipe_mutex);

#ifdef ENABLE_WASM
    wasm_mgr_cleanup();
#endif

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
    uv_mutex_unlock(&engine->pipe_mutex);
    return p;
}

void stream_engine_resume(pipeline_t *p) {
    if (!p || p->state != PIPE_STATE_SUSPENDED) return;
    p->state = PIPE_STATE_RUNNING;
    pipeline_stage_t *s = p->resume_point ? p->resume_point->next[0] : NULL;
    while (s) {
        if (s->health != STAGE_HEALTH_GOOD) {
            s = s->next[0];
            continue;
        }
        stage_ret_t ret = s->process(s, &p->current_event);
        if (ret == STAGE_ASYNC) {
            p->state = PIPE_STATE_SUSPENDED;
            p->resume_point = s;
            return;
        }
        if (ret == STAGE_DROP || ret == STAGE_ERROR) break;
        
        if (ret >= STAGE_BRANCH_0 && ret <= STAGE_BRANCH_3) {
            s = s->next[ret - STAGE_BRANCH_0];
        } else {
            s = s->next[0];
        }
    }
    p->state = PIPE_STATE_IDLE;
    p->resume_point = NULL;
}

int stream_engine_checkpoint_save(stream_engine_t *engine, sqlite_svc_t *db) {
    if (!engine || !db) return -1;
    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uint8_t state_buf[512];
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        pipeline_stage_t *s = p->head;
        int stage_idx = 0;
        while (s) {
            if (s->save_state) {
                int len = s->save_state(s, state_buf, sizeof(state_buf));
                if (len > 0) {
                    char product_key[128];
                    snprintf(product_key, sizeof(product_key), "STATE:%s", p->id);
                    sqlite_svc_set_param(db, product_key, (uint32_t)stage_idx, state_buf, (size_t)len);
                }
            }
            s = s->next[0];
            stage_idx++;
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
    return 0;
}

int stream_engine_checkpoint_restore(stream_engine_t *engine, sqlite_svc_t *db) {
    if (!engine || !db) return -1;
    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uint8_t state_buf[512];
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        pipeline_stage_t *s = p->head;
        int stage_idx = 0;
        while (s) {
            if (s->load_state) {
                char product_key[128];
                snprintf(product_key, sizeof(product_key), "STATE:%s", p->id);
                int len = sqlite_svc_get_param(db, product_key, (uint32_t)stage_idx, state_buf, sizeof(state_buf));
                if (len > 0) {
                    s->load_state(s, state_buf, (size_t)len);
                }
            }
            s = s->next[0];
            stage_idx++;
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
    return 0;
}

stage_health_t pipeline_stage_get_health(pipeline_stage_t *stage) {
    return stage ? stage->health : STAGE_HEALTH_GOOD;
}

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg) {
    if (!engine || !engine->hub) return;
    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        if (_is_pipeline_triggered(p, msg->tag_id)) {
            _pipeline_process(p, msg);
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}

uv_async_t *stream_engine_get_notify_handle(stream_engine_t *engine) {
    return engine ? &engine->notify_handle : NULL;
}
