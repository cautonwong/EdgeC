#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algo_peak_shaving.h"
#include "battery_strategy.h"
#include "edge_log.h"
#include "sqlite_service.h"
#include "stream_engine.h"

// --- 1. Filter ---
static stage_ret_t _filter_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    return (ev->tag_id == (uint32_t)(uintptr_t)s->priv_cfg) ? STAGE_OK : STAGE_DROP;
}
pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = _filter_proc;
    s->priv_cfg = (void *)(uintptr_t)tag_id;
    return s;
}

// --- 2. Filter Range ---
typedef struct {
    double min, max;
} _range_cfg_t;
static stage_ret_t _range_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _range_cfg_t *c = s->priv_cfg;
    return (ev->payload.val.f < c->min || ev->payload.val.f > c->max) ? STAGE_DROP : STAGE_OK;
}
pipeline_stage_t *stage_filter_range_create(double min, double max) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter_range", 31);
    s->process = _range_proc;
    _range_cfg_t *c = malloc(sizeof(_range_cfg_t));
    c->min = min;
    c->max = max;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

// --- 3. Transform ---
typedef struct {
    double a, b;
} _trans_cfg_t;
static stage_ret_t _trans_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _trans_cfg_t *c = s->priv_cfg;
    ev->payload.val.f = ev->payload.val.f * c->a + c->b;
    return STAGE_OK;
}
pipeline_stage_t *stage_transform_create(double alpha, double beta) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "transform", 31);
    s->process = _trans_proc;
    _trans_cfg_t *c = malloc(sizeof(_trans_cfg_t));
    c->a = alpha;
    c->b = beta;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

// --- 4. EMA ---
typedef struct {
    double alpha;
} _ema_cfg_t;
typedef struct {
    double last;
    bool inited;
} _ema_st_t;
static stage_ret_t _ema_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _ema_cfg_t *c = s->priv_cfg;
    _ema_st_t *st = s->priv_state;
    if (!st->inited) {
        st->last = ev->payload.val.f;
        st->inited = true;
    } else {
        st->last = c->alpha * ev->payload.val.f + (1.0 - c->alpha) * st->last;
    }
    ev->payload.val.f = st->last;
    return STAGE_OK;
}
pipeline_stage_t *stage_ema_create(double alpha) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "ema", 31);
    s->process = _ema_proc;
    _ema_cfg_t *c = malloc(sizeof(_ema_cfg_t));
    c->alpha = alpha;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    s->priv_state = calloc(1, sizeof(_ema_st_t));
    s->cleanup_state = free;
    return s;
}

// --- 5. Clamp ---
typedef struct {
    double min, max;
} _clamp_cfg_t;
static stage_ret_t _clamp_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _clamp_cfg_t *c = s->priv_cfg;
    if (ev->payload.val.f < c->min) ev->payload.val.f = c->min;
    if (ev->payload.val.f > c->max) ev->payload.val.f = c->max;
    return STAGE_OK;
}
pipeline_stage_t *stage_clamp_create(double min, double max) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "clamp", 31);
    s->process = _clamp_proc;
    _clamp_cfg_t *c = malloc(sizeof(_clamp_cfg_t));
    c->min = min;
    c->max = max;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

// --- 6. Peak Shaving ---
typedef struct {
    peak_shave_config_t algo;
    uint32_t ctrl_tag;
} _shave_cfg_t;
static stage_ret_t _shave_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _shave_cfg_t *c = s->priv_cfg;
    double target = algo_peak_shave_calculate(ev->payload.val.f, &c->algo);
    if (target != 0.0) {
        ev->tag_id = c->ctrl_tag;
        ev->payload.val.f = target;
        return STAGE_OK;
    }
    return STAGE_DROP;
}
pipeline_stage_t *stage_peak_shaving_create(double threshold, uint32_t ctrl_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "peak_shave", 31);
    s->process = _shave_proc;
    _shave_cfg_t *c = calloc(1, sizeof(_shave_cfg_t));
    c->algo.upper_threshold = threshold;
    c->algo.max_discharge = 5000.0;
    c->ctrl_tag = ctrl_tag;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

// --- 7. Accumulate ---
static stage_ret_t _acc_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    *(double *)s->priv_state += ev->payload.val.f;
    return STAGE_OK;
}
pipeline_stage_t *stage_accumulate_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = _acc_proc;
    s->priv_state = calloc(1, sizeof(double));
    s->cleanup_state = free;
    return s;
}

// --- 8. Periodic Freeze ---
typedef struct {
    double acc;
    uint32_t n;
    int kind;
} _freeze_st_t;
static stage_ret_t _freeze_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    _freeze_st_t *st = s->priv_state;
    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        st->acc += ev->payload.val.f;
        st->n++;
        return STAGE_OK;
    }
    if (ev->type == PIPEMSG_TYPE_EVENT && ev->payload.event.param == (uint32_t)st->kind) {
        ev->type = PIPEMSG_SCALAR_F64;
        ev->payload.val.f = (st->n > 0) ? (st->acc / st->n) : 0;
        st->acc = 0;
        st->n = 0;
        return STAGE_OK;
    }
    return STAGE_DROP;
}
pipeline_stage_t *stage_periodic_freeze_create(int kind) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "freeze", 31);
    s->process = _freeze_proc;
    _freeze_st_t *st = calloc(1, sizeof(_freeze_st_t));
    st->kind = kind;
    s->priv_state = st;
    s->cleanup_state = free;
    return s;
}

// --- 9. TimeGuard ---
typedef struct {
    uint64_t last_mono, last_cal;
    bool inited;
} _tg_st_t;
static stage_ret_t _tg_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    _tg_st_t *st = s->priv_state;
    uint64_t now = uv_hrtime() / 1000000;
    if (!st->inited) {
        st->last_mono = now;
        st->last_cal = ev->ts;
        st->inited = true;
        return STAGE_OK;
    }
    int64_t drift = (int64_t)(ev->ts - st->last_cal) - (int64_t)(now - st->last_mono);
    if (drift > 5000 || drift < -5000) ev->quality = 0x01;
    st->last_mono = now;
    st->last_cal = ev->ts;
    return STAGE_OK;
}
pipeline_stage_t *stage_time_guard_create(void) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "time_guard", 31);
    s->process = _tg_proc;
    s->priv_state = calloc(1, sizeof(_tg_st_t));
    s->cleanup_state = free;
    return s;
}

// --- 10. SQLite Sink (Asynchronous) ---

typedef struct {
    sqlite_svc_t *db;
    char table[64];
} _sink_cfg_t;

// Context for the background work request
typedef struct {
    uv_work_t req;
    pipeline_t *pipeline;  // To resume it later
    sqlite_svc_t *db;
    char *sql;
} _sink_work_ctx_t;

// This runs on a background worker thread
static void _sink_work_cb(uv_work_t *req) {
    _sink_work_ctx_t *ctx = req->data;
    sqlite_svc_exec(ctx->db, ctx->sql, NULL, NULL);
}

// This runs on the compute thread's loop after the work is done
static void _sink_after_work_cb(uv_work_t *req, int status) {
    (void)status;
    _sink_work_ctx_t *ctx = req->data;
    // Resume the pipeline that was paused
    stream_engine_resume(ctx->pipeline);
    // Cleanup
    free(ctx->sql);
    free(ctx);
}

static stage_ret_t _sink_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;

    _sink_cfg_t *c = s->priv_cfg;

    _sink_work_ctx_t *work_ctx = malloc(sizeof(_sink_work_ctx_t));
    work_ctx->req.data = work_ctx;
    work_ctx->pipeline = s->pipeline;  // The stage now knows its parent pipeline
    work_ctx->db = c->db;

    char sql_buf[256];
    snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s (tag_id, value, ts) VALUES (%u, %f, %lu)",
             c->table, ev->tag_id, ev->payload.val.f, ev->ts);
    work_ctx->sql = strdup(sql_buf);

    // Queue the work to run on a background thread
    uv_queue_work(s->loop, &work_ctx->req, _sink_work_cb, _sink_after_work_cb);

    // Pause this pipeline until the work is done
    return STAGE_ASYNC;
}

pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    if (!db || !table) return NULL;

    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = _sink_proc;

    _sink_cfg_t *c = malloc(sizeof(_sink_cfg_t));
    c->db = db;
    strncpy(c->table, table, 63);
    s->priv_cfg = c;
    s->cleanup_cfg = free;

    // Ensure the table exists
    char sql[256];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s (tag_id INTEGER, value REAL, ts INTEGER)", table);
    sqlite_svc_exec(db, sql, NULL, NULL);

    return s;
}

// --- 11. Battery Guard ---
static stage_ret_t _batt_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    battery_limit_t *lm = s->priv_cfg;
    // ev->payload.val.f = algo_battery_safety_clamp(NULL, lm, ev->payload.val.f);
    (void)lm;
    return STAGE_OK;
}
pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "batt_guard", 31);
    s->process = _batt_proc;
    s->priv_cfg = limits;
    s->cleanup_cfg = free;
    return s;
}

// --- 12. Reflex Logic (Sprint 3) ---
#include "arbiter.h"
#include "bus_orchestrator.h"

typedef struct {
    struct bus_orchestrator_s *orch;
    uint32_t bus_id;
    char proto[16];
    uint32_t target_tag;
    double threshold;
    bool greater_than;
} _reflex_cfg_t;

static stage_ret_t _reflex_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _reflex_cfg_t *c = s->priv_cfg;

    bool triggered =
        c->greater_than ? (ev->payload.val.f > c->threshold) : (ev->payload.val.f < c->threshold);

    if (triggered) {
        LOG_WARN(LOG_CAT_BUS, "Reflex: Threshold hit! Triggering Tag %u on Bus %d [%s]",
                 c->target_tag, c->bus_id, c->proto);

        cmd_msg_t cmd = {0};
        cmd.action = CMD_ACTION_WRITE;
        cmd.tag_id = c->target_tag;
        cmd.value = 1.0;  // 默认触发动作
        cmd.priority = ARBITER_PRIO_HIGH;
        strncpy(cmd.protocol, c->proto, 15);
        snprintf(cmd.req_id, sizeof(cmd.req_id), "REFLEX_%u", c->target_tag);

        bus_orchestrator_submit_cmd(c->orch, (int)c->bus_id, &cmd);
    }

    return STAGE_OK;
}

pipeline_stage_t *stage_reflex_logic_create(struct bus_orchestrator_s *orch, uint32_t bus_id,
                                            const char *proto, uint32_t target_tag,
                                            double threshold, bool greater_than) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "reflex", 31);
    s->process = _reflex_proc;
    _reflex_cfg_t *c = calloc(1, sizeof(_reflex_cfg_t));
    c->orch = orch;
    c->bus_id = bus_id;
    strncpy(c->proto, proto, 15);
    c->target_tag = target_tag;
    c->threshold = threshold;
    c->greater_than = greater_than;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

// --- 13. Moving Average (Sprint 3) ---
typedef struct {
    double *window;
    uint32_t size;
    uint32_t head;
    uint32_t count;
    double sum;
} _ma_st_t;

static void _ma_cleanup(void *p) {
    _ma_st_t *st = p;
    if (st->window) free(st->window);
    free(st);
}

static stage_ret_t _ma_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _ma_st_t *st = s->priv_state;

    if (st->count < st->size) {
        st->window[st->head] = ev->payload.val.f;
        st->sum += ev->payload.val.f;
        st->count++;
    } else {
        st->sum -= st->window[st->head];
        st->window[st->head] = ev->payload.val.f;
        st->sum += ev->payload.val.f;
    }

    st->head = (st->head + 1) % st->size;
    ev->payload.val.f = st->sum / st->count;
    return STAGE_OK;
}

pipeline_stage_t *stage_moving_average_create(uint32_t window_size) {
    if (window_size == 0) window_size = 1;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "moving_average", 31);
    s->process = _ma_proc;
    _ma_st_t *st = calloc(1, sizeof(_ma_st_t));
    st->size = window_size;
    st->window = calloc(window_size, sizeof(double));
    s->priv_state = st;
    s->cleanup_state = _ma_cleanup;
    return s;
}

// --- 14. RBE (Report By Exception) (Sprint 3) ---
typedef struct {
    double threshold_pct;
    double last_reported;
    bool has_last;
} _rbe_st_t;

static stage_ret_t _rbe_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _rbe_st_t *st = s->priv_state;

    if (!st->has_last) {
        st->last_reported = ev->payload.val.f;
        st->has_last = true;
        return STAGE_OK;
    }

    double diff = fabs(ev->payload.val.f - st->last_reported);
    double threshold = fabs(st->last_reported * (st->threshold_pct / 100.0));

    if (diff >= threshold) {
        st->last_reported = ev->payload.val.f;
        return STAGE_OK;
    }

    return STAGE_DROP;
}

pipeline_stage_t *stage_rbe_create(double threshold_pct) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "rbe", 31);
    s->process = _rbe_proc;
    _rbe_st_t *st = calloc(1, sizeof(_rbe_st_t));
    st->threshold_pct = threshold_pct;
    s->priv_state = st;
    s->cleanup_state = free;
    return s;
}
