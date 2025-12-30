#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite_service.h"
#include "stream_engine.h"

// --- 1. Stage: Filter ---
stage_ret_t stage_tag_filter_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    uint32_t target_tag = (uint32_t)(uintptr_t)stage->priv_cfg;
    return (ev->tag_id == target_tag) ? STAGE_OK : STAGE_DROP;
}

pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = stage_tag_filter_proc;
    s->priv_cfg = (void *)(uintptr_t)tag_id;
    return s;
}

// --- 2. Stage: Transform ---
typedef struct {
    double alpha;
    double beta;
} transform_cfg_t;
stage_ret_t stage_transform_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    transform_cfg_t *cfg = (transform_cfg_t *)stage->priv_cfg;
    ev->payload.val = ev->payload.val * cfg->alpha + cfg->beta;
    return STAGE_OK;
}

pipeline_stage_t *stage_transform_create(double alpha, double beta) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "transform", 31);
    s->process = stage_transform_proc;
    transform_cfg_t *cfg = malloc(sizeof(transform_cfg_t));
    cfg->alpha = alpha;
    cfg->beta = beta;
    s->priv_cfg = cfg;
    s->cleanup_cfg = free;
    return s;
}

// --- 3. Stage: Filter Range ---
typedef struct {
    double min;
    double max;
} filter_range_cfg_t;
stage_ret_t stage_filter_range_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    double val = ev->payload.val;
    filter_range_cfg_t *cfg = (filter_range_cfg_t *)stage->priv_cfg;

    // IEEE 754 防御：NaN 不满足任何比较条件，必须显式拦截
    if (isnan(val) || val < cfg->min || val > cfg->max) {
        return STAGE_DROP;
    }
    return STAGE_OK;
}

pipeline_stage_t *stage_filter_range_create(double min, double max) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter_range", 31);
    s->process = stage_filter_range_proc;
    filter_range_cfg_t *cfg = malloc(sizeof(filter_range_cfg_t));
    cfg->min = min;
    cfg->max = max;
    s->priv_cfg = cfg;
    s->cleanup_cfg = free;
    return s;
}

// --- 4. Stage: Accumulate ---
typedef struct {
    double acc;
} acc_state_t;
stage_ret_t stage_accumulator_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    ((acc_state_t *)stage->priv_state)->acc += ev->payload.val;
    return STAGE_OK;
}

pipeline_stage_t *stage_accumulate_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = stage_accumulator_proc;
    s->priv_state = calloc(1, sizeof(acc_state_t));
    s->cleanup_state = free;
    return s;
}

// --- 5. Stage: SQLite Sink ---
typedef struct {
    sqlite_svc_t *db;
    char table[64];
} sink_cfg_t;
typedef struct {
    sqlite_svc_t *db;
    PipeMsg msg;
    uv_work_t req;
} sink_work_t;
static void sink_work_cb(uv_work_t *req) {
    sqlite_svc_insert(((sink_work_t *)req->data)->db, &((sink_work_t *)req->data)->msg);
}
static void sink_after_cb(uv_work_t *req, int status) {
    (void)status;
    free(req->data);
}

stage_ret_t stage_sqlite_sink_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    sink_cfg_t *cfg = (sink_cfg_t *)stage->priv_cfg;
    if (!cfg || !cfg->db) return STAGE_OK;
    sink_work_t *work = calloc(1, sizeof(sink_work_t));
    work->db = cfg->db;
    work->msg.tag_id = ev->tag_id;
    work->msg.payload.val.f = ev->payload.val;
    work->msg.ts = ev->calendar_ms;
    work->req.data = work;
    uv_queue_work(uv_default_loop(), &work->req, sink_work_cb, sink_after_cb);
    return STAGE_OK;
}

static void sink_cleanup(void *ptr) {
    if (ptr) free(ptr);
}
pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = stage_sqlite_sink_proc;
    sink_cfg_t *cfg = malloc(sizeof(sink_cfg_t));
    cfg->db = db;
    if (table) strncpy(cfg->table, table, 63);
    s->priv_cfg = cfg;
    s->cleanup_cfg = sink_cleanup;
    return s;
}
