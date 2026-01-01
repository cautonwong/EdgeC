#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"
#include "stream_engine_priv.h"

// 内部：运行 Pipeline 逻辑
static void _run_pipeline(pipeline_t *p, pipe_msg_t *msg) {
    if (!p || !msg) return;
    pipeline_stage_t *curr = p->head;
    while (curr) {
        stage_ret_t ret = curr->process(curr, msg);
        if (ret == STAGE_DROP || ret == STAGE_ERROR) break;
        if (ret == STAGE_ASYNC) break;
        curr = curr->next;
    }
}

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    uv__queue_init(&engine->pipelines);
    uv_mutex_init(&engine->pipe_mutex);

    if (time_src) {
        engine->time_src = *time_src;
    }

    edge_log_init(NULL, true);
    return engine;
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;

    uv_mutex_lock(&engine->pipe_mutex);
    while (!uv__queue_empty(&engine->pipelines)) {
        struct uv__queue *q = uv__queue_head(&engine->pipelines);
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        uv__queue_remove(q);

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
    uv_mutex_unlock(&engine->pipe_mutex);

    uv_mutex_destroy(&engine->pipe_mutex);
    free(engine);
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
        p->head = p->tail = s;
    } else {
        p->tail->next = s;
        p->tail = s;
    }
}

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg) {
    if (!engine || !msg) return;

    uv_mutex_lock(&engine->pipe_mutex);
    struct uv__queue *q;
    uv__queue_foreach(q, &engine->pipelines) {
        pipeline_t *p = uv__queue_data(q, pipeline_t, node);
        if (p->trigger_tag == 0 || p->trigger_tag == msg->tag_id) {
            _run_pipeline(p, msg);
        }
    }
    uv_mutex_unlock(&engine->pipe_mutex);
}
