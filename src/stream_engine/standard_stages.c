#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery_strategy.h"
#include "sqlite_service.h"
#include "stream_engine.h"

// --- 1. Stage: Tag Filter ---
static stage_ret_t _tag_filter_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    uint32_t target = (uint32_t)(uintptr_t)stage->priv_cfg;
    return (ev->tag_id == target) ? STAGE_OK : STAGE_DROP;
}

pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = _tag_filter_proc;
    s->priv_cfg = (void *)(uintptr_t)tag_id;
    return s;
}

// --- 2. Stage: Filter Range ---
typedef struct {
    double min;
    double max;
} _range_cfg_t;
static stage_ret_t _filter_range_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _range_cfg_t *cfg = (_range_cfg_t *)stage->priv_cfg;
    return (ev->payload.val.f < cfg->min || ev->payload.val.f > cfg->max) ? STAGE_DROP : STAGE_OK;
}

pipeline_stage_t *stage_filter_range_create(double min, double max) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter_range", 31);
    s->process = _filter_range_proc;
    _range_cfg_t *cfg = malloc(sizeof(_range_cfg_t));
    cfg->min = min;
    cfg->max = max;
    s->priv_cfg = cfg;
    s->cleanup_cfg = free;
    return s;
}

// --- 3. Stage: Transform ---
typedef struct {
    double a;
    double b;
} _trans_cfg_t;
static stage_ret_t _transform_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _trans_cfg_t *cfg = (_trans_cfg_t *)stage->priv_cfg;
    ev->payload.val.f = ev->payload.val.f * cfg->a + cfg->b;
    return STAGE_OK;
}

pipeline_stage_t *stage_transform_create(double alpha, double beta) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "transform", 31);
    s->process = _transform_proc;
    _trans_cfg_t *cfg = malloc(sizeof(_trans_cfg_t));
    cfg->a = alpha;
    cfg->b = beta;
    s->priv_cfg = cfg;
    s->cleanup_cfg = free;
    return s;
}

// --- 4. Stage: Accumulate ---
typedef struct {
    double acc;
} _acc_state_t;
static stage_ret_t _acc_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    ((_acc_state_t *)stage->priv_state)->acc += ev->payload.val.f;
    return STAGE_OK;
}

pipeline_stage_t *stage_accumulate_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = _acc_proc;
    s->priv_state = calloc(1, sizeof(_acc_state_t));
    s->cleanup_state = free;
    return s;
}

// --- 5. Stage: SQLite Sink ---
typedef struct {
    sqlite_svc_t *db;
    char table[64];
} _sink_cfg_t;
typedef struct {
    sqlite_svc_t *db;
    PipeMsg msg;
    uv_work_t req;
} _sink_work_t;
static void _sink_work_cb(uv_work_t *req) {
    sqlite_svc_insert(((_sink_work_t *)req->data)->db, &((_sink_work_t *)req->data)->msg);
}
static void _sink_after_cb(uv_work_t *req, int status) {
    (void)status;
    free(req->data);
}

static stage_ret_t _sink_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _sink_cfg_t *cfg = (_sink_cfg_t *)stage->priv_cfg;
    if (!cfg || !cfg->db) return STAGE_OK;
    _sink_work_t *work = calloc(1, sizeof(_sink_work_t));
    work->db = cfg->db;
    work->msg.tag_id = ev->tag_id;
    work->msg.payload.val.f = ev->payload.val.f;
    work->msg.ts = ev->ts;
    work->req.data = work;
    uv_queue_work(uv_default_loop(), &work->req, _sink_work_cb, _sink_after_cb);
    return STAGE_OK;
}

pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = _sink_proc;
    _sink_cfg_t *cfg = malloc(sizeof(_sink_cfg_t));
    cfg->db = db;
    if (table) strncpy(cfg->table, table, 63);
    s->priv_cfg = cfg;
    s->cleanup_cfg = free;
    return s;
}

// --- 6. Stage: Periodic Freeze ---
typedef struct {
    double acc;
    uint32_t count;
    int kind;
} _freeze_state_t;
static stage_ret_t _freeze_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    _freeze_state_t *s = (_freeze_state_t *)stage->priv_state;
    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        s->acc += ev->payload.val.f;
        s->count++;
        return STAGE_OK;
    }
    if (ev->type == PIPEMSG_TYPE_EVENT && ev->payload.event.param == (uint32_t)s->kind) {
        ev->type = PIPEMSG_TYPE_SCALAR;
        ev->payload.val.f = (s->count > 0) ? (s->acc / s->count) : 0;
        s->acc = 0;
        s->count = 0;
        return STAGE_OK;
    }
    return STAGE_DROP;
}

pipeline_stage_t *stage_periodic_freeze_create(int kind) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "periodic_freeze", 31);
    s->process = _freeze_proc;
    _freeze_state_t *fs = calloc(1, sizeof(_freeze_state_t));
    fs->kind = kind;
    s->priv_state = fs;
    s->cleanup_state = free;
    return s;
}

// --- 7. Stage: Battery Guard ---
static stage_ret_t _batt_guard_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    battery_state_t *st = (battery_state_t *)stage->priv_state;
    battery_limit_t *lm = (battery_limit_t *)stage->priv_cfg;
    if (!st || !lm) return STAGE_OK;
    ev->payload.val.f = (double)algo_battery_safety_clamp(st, lm, (float)ev->payload.val.f);
    return STAGE_OK;
}

pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "battery_guard", 31);
    s->process = _batt_guard_proc;
    s->priv_cfg = limits;
    s->cleanup_cfg = free;
    return s;
}
