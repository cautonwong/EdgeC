/**
 * @file stream_engine.c
 * @brief 流处理引擎核心实现 (支持 S17 逻辑熔断)
 */

#include "stream_engine.h"

#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define DEFAULT_CIRCUIT_BREAKER_US 50000  // 默认 50ms 熔断

struct pipeline_s {
    char id[64];
    uint32_t trigger_tag;
    pipeline_stage_t *head;
    pipeline_stage_t *tail;
    struct pipeline_s *next;
};

struct stream_engine_s {
    datahub_t *hub;
    engine_time_source_t *time_src;
    pipeline_t *pipelines;
    uint64_t seq;
};

static void _pipeline_process(pipeline_t *p, pipe_msg_t *msg) {
    pipeline_stage_t *s = p->head;
    while (s) {
        // 如果算子已熔断，直接跳过
        if (s->health != STAGE_HEALTH_GOOD) {
            s = s->next;
            continue;
        }

        uint64_t start = uv_hrtime();
        stage_ret_t ret = s->process(s, msg);
        uint64_t elapsed_us = (uv_hrtime() - start) / 1000;

        // 更新统计
        s->last_proc_us = elapsed_us;
        s->total_events++;

        // S17: 检查逻辑熔断
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

static void _on_data(datahub_t *hub, void *user_data) {
    stream_engine_t *engine = user_data;
    pipe_msg_t msg;

    while (shm_ring_read(&hub->ring, &engine->seq, &msg) == 0) {
        pipeline_t *p = engine->pipelines;
        while (p) {
            if (p->trigger_tag == msg.tag_id || p->trigger_tag == 0) {
                _pipeline_process(p, &msg);
            }
            p = p->next;
        }
    }
}

stream_engine_t *stream_engine_create(datahub_t *hub, engine_time_source_t *time_src) {
    stream_engine_t *engine = calloc(1, sizeof(stream_engine_t));
    engine->hub = hub;
    engine->time_src = time_src;

    datahub_subscribe(hub, _on_data, engine);
    LOG_INFO(LOG_CAT_SCHED, "Stream Engine operational.");
    return engine;
}

pipeline_stage_t *pipeline_add_stage(pipeline_t *p, const char *name, stage_proc_fn proc,
                                     void *priv_state, stage_cleanup_fn cleanup_state,
                                     void *priv_cfg, stage_cleanup_fn cleanup_cfg) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, name, 31);
    s->process = proc;
    s->priv_state = priv_state;
    s->priv_cfg = priv_cfg;
    s->cleanup_state = cleanup_state;
    s->cleanup_cfg = cleanup_cfg;
    s->health = STAGE_HEALTH_GOOD;
    s->circuit_breaker_us = DEFAULT_CIRCUIT_BREAKER_US;

    if (!p->head) {
        p->head = p->tail = s;
    } else {
        p->tail->next = s;
        p->tail = s;
    }
    return s;
}

pipeline_t *stream_engine_add_pipeline(stream_engine_t *engine, const char *id,
                                       uint32_t trigger_tag) {
    pipeline_t *p = calloc(1, sizeof(pipeline_t));
    strncpy(p->id, id, 63);
    p->trigger_tag = trigger_tag;
    p->next = engine->pipelines;
    engine->pipelines = p;
    return p;
}

void stream_engine_destroy(stream_engine_t *engine) {
    if (!engine) return;

    // S8: 物理断开总线订阅
    datahub_unsubscribe(engine->hub);

    pipeline_t *p = engine->pipelines;
    while (p) {
        pipeline_t *next_p = p->next;
        pipeline_stage_t *s = p->head;
        while (s) {
            pipeline_stage_t *next_s = s->next;
            if (s->cleanup_state) s->cleanup_state(s->priv_state);
            if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
            free(s);
            s = next_s;
        }
        free(p);
        p = next_p;
    }
    free(engine);
}

void stream_engine_push(stream_engine_t *engine, pipe_msg_t *msg) {
    pipeline_t *p = engine->pipelines;

    while (p) {
        if (p->trigger_tag == msg->tag_id || p->trigger_tag == 0) {
            _pipeline_process(p, msg);
        }

        p = p->next;
    }
}

stage_health_t pipeline_stage_get_health(pipeline_stage_t *stage) {
    return stage ? stage->health : STAGE_HEALTH_GOOD;
}
