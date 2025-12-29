#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite_service.h"
#include "stream_engine.h"

// --- Stage: Tag Filter ---
stage_ret_t stage_tag_filter(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    // 强制转换为 uint32_t，无需额外 malloc (Principles 1: efficient)
    uint32_t target_tag = (uint32_t)(uintptr_t)stage->priv_cfg;
    return (ev->tag_id == target_tag) ? STAGE_OK : STAGE_DROP;
}

// --- Stage: Accumulate (Stateless) ---
typedef struct {
    double acc;
    uint32_t count;
} acc_state_t;

stage_ret_t stage_accumulator(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    acc_state_t *state = (acc_state_t *)stage->priv_state;
    state->acc += ev->payload.val;
    state->count++;
    return STAGE_OK;
}

// --- Stage: SQLite Sink ---
typedef struct {
    sqlite_svc_t *db;
    PipeMsg msg;
    uv_work_t req;
    pipeline_t *pipe;
} sink_work_ctx_t;

static void sqlite_sink_work(uv_work_t *req) {
    sink_work_ctx_t *ctx = (sink_work_ctx_t *)req->data;
    sqlite_svc_insert(ctx->db, &ctx->msg);
}

static void sqlite_sink_after(uv_work_t *req, int status) {
    (void)status;
    sink_work_ctx_t *ctx = (sink_work_ctx_t *)req->data;
    // 此处需要回调 pipeline_resume，但注意并发安全
    // 实际生产中应通过 uv_async 唤醒引擎 loop 执行 resume
    free(ctx);
}

stage_ret_t stage_sqlite_sink(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    sqlite_svc_t *db = (sqlite_svc_t *)stage->priv_cfg;
    if (!db) return STAGE_OK;

    sink_work_ctx_t *ctx = calloc(1, sizeof(sink_work_ctx_t));
    ctx->db = db;
    ctx->msg.tag_id = ev->tag_id;
    ctx->msg.payload.val.f = ev->payload.val;
    ctx->msg.ts = ev->calendar_ms;
    ctx->req.data = ctx;

    uv_queue_work(uv_default_loop(), &ctx->req, sqlite_sink_work, sqlite_sink_after);
    return STAGE_OK;  // 暂时简化为同步完成以验证 loader 逻辑
}

// --- Factories: 交付属性包而非堆对象 ---

pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = stage_tag_filter;
    s->priv_cfg = (void *)(uintptr_t)tag_id;
    return s;
}

pipeline_stage_t *stage_accumulate_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = stage_accumulator;
    s->priv_state = calloc(1, sizeof(acc_state_t));
    s->cleanup = free;
    return s;
}

pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    (void)table;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = stage_sqlite_sink;
    s->priv_cfg = db;
    return s;
}
