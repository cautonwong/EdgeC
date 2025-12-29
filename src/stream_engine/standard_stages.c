#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite_service.h"
#include "stream_engine.h"

// --- 1. Filter Stage ---
typedef struct {
    uint32_t target_tag;
} filter_cfg_t;

static stage_ret_t filter_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    filter_cfg_t *cfg = (filter_cfg_t *)stage->priv_cfg;
    if (ev->type == EVT_DATA_POINT && ev->tag_id != cfg->target_tag) {
        return STAGE_DROP;
    }
    return STAGE_OK;
}

pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = filter_proc;
    filter_cfg_t *cfg = malloc(sizeof(filter_cfg_t));
    cfg->target_tag = tag_id;
    s->priv_cfg = cfg;
    s->cleanup = free;
    return s;
}

// --- 2. Accumulate Stage (Stateless calculation, state in priv_state) ---
typedef struct {
    double sum;
    uint64_t count;
} acc_state_t;

static stage_ret_t acc_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    acc_state_t *state = (acc_state_t *)stage->priv_state;
    state->sum += ev->payload.val;
    state->count++;
    return STAGE_OK;
}

pipeline_stage_t *stage_accumulate_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = acc_proc;
    s->priv_state = calloc(1, sizeof(acc_state_t));
    s->cleanup = free;
    return s;
}

// --- 3. SQLite Sink Stage (Asynchronous) ---
typedef struct {
    sqlite_svc_t *db;
    char table[64];
} sink_cfg_t;

typedef struct {
    sqlite_svc_t *db;
    PipeMsg msg;
} sink_work_t;

static void sqlite_sink_work(uv_work_t *req) {
    sink_work_t *work = (sink_work_t *)req->data;
    // Real industrial logic: write to persistent ledger
    sqlite_svc_insert(work->db, &work->msg);
}

static void sqlite_sink_after(uv_work_t *req, int status) {
    (void)status;
    sink_work_t *work = (sink_work_t *)req->data;
    free(work);
    free(req);
}

static stage_ret_t sink_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    sink_cfg_t *cfg = (sink_cfg_t *)stage->priv_cfg;

    // Principles 1: Use libuv work queue for blocking DB IO
    uv_work_t *req = malloc(sizeof(uv_work_t));
    sink_work_t *work = malloc(sizeof(sink_work_t));
    work->db = cfg->db;
    work->msg.tag_id = ev->tag_id;
    work->msg.payload.val.f = ev->payload.val;
    work->msg.ts = ev->calendar_ms;

    req->data = work;
    uv_queue_work(uv_default_loop(), req, sqlite_sink_work, sqlite_sink_after);

    return STAGE_OK;
}

pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = sink_proc;
    sink_cfg_t *cfg = malloc(sizeof(sink_cfg_t));
    cfg->db = db;
    strncpy(cfg->table, table, 63);
    s->priv_cfg = cfg;
    s->cleanup = free;
    return s;
}
