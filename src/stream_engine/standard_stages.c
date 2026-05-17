#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "algo_peak_shaving.h"
#include "battery_strategy.h"
#include "edge_log.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_dsp.h"
#include "edge_control.h"
#include "edge_motor.h"
#include "arbiter.h"
#include "bus_orchestrator.h"

// --- Private Config Structs ---

typedef struct {
    double min, max;
} _range_cfg_t;

typedef struct {
    double a, b;
} _trans_cfg_t;

typedef struct {
    double alpha;
} _ema_cfg_t;

typedef struct {
    double last;
    bool inited;
} _ema_st_t;

typedef struct {
    double min, max;
} _clamp_cfg_t;

typedef struct {
    peak_shave_config_t algo;
    uint32_t ctrl_tag;
} _shave_cfg_t;

typedef struct {
    double acc;
    uint32_t n;
    int kind;
} _freeze_st_t;

typedef struct {
    uint64_t last_mono, last_cal;
    bool inited;
} _tg_st_t;

typedef struct {
    sqlite_svc_t *db;
    char table[64];
} _sink_cfg_t;

typedef struct {
    uv_work_t req;
    pipeline_t *pipeline;
    sqlite_svc_t *db;
    char *sql;
} _sink_work_ctx_t;

typedef struct {
    struct bus_orchestrator_s *orch;
    uint32_t bus_id;
    char proto[16];
    uint32_t target_tag;
    double threshold;
    bool greater_than;
    double action_value;
} _reflex_cfg_t;

typedef struct {
    double *window;
    uint32_t size;
    uint32_t head;
    uint32_t count;
    double sum;
} _ma_st_t;

typedef struct {
    double threshold_pct;
    double last_reported;
    bool has_last;
} _rbe_st_t;

typedef struct {
    control_kf_t kf;
    uint32_t state_dim;
    uint32_t meas_dim;
} _kf_st_t;

// --- Process Callbacks ---

static stage_ret_t _filter_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    return (ev->tag_id == (uint32_t)(uintptr_t)s->priv_cfg) ? STAGE_OK : STAGE_DROP;
}

static stage_ret_t _range_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _range_cfg_t *c = s->priv_cfg;
    return (ev->payload.val.f < c->min || ev->payload.val.f > c->max) ? STAGE_DROP : STAGE_OK;
}

static stage_ret_t _trans_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _trans_cfg_t *c = s->priv_cfg;
    ev->payload.val.f = ev->payload.val.f * c->a + c->b;
    return STAGE_OK;
}

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

static stage_ret_t _clamp_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _clamp_cfg_t *c = s->priv_cfg;
    if (ev->payload.val.f < c->min) ev->payload.val.f = c->min;
    if (ev->payload.val.f > c->max) ev->payload.val.f = c->max;
    return STAGE_OK;
}

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

static stage_ret_t _acc_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    *(double *)s->priv_state += ev->payload.val.f;
    return STAGE_OK;
}

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

static void _sink_work_cb(uv_work_t *req) {
    _sink_work_ctx_t *ctx = req->data;
    sqlite_svc_exec(ctx->db, ctx->sql, NULL, NULL);
}

static void _sink_after_work_cb(uv_work_t *req, int status) {
    (void)status;
    _sink_work_ctx_t *ctx = req->data;
    stream_engine_resume(ctx->pipeline);
    free(ctx->sql);
    free(ctx);
}

static stage_ret_t _sink_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _sink_cfg_t *c = s->priv_cfg;
    _sink_work_ctx_t *work_ctx = malloc(sizeof(_sink_work_ctx_t));
    work_ctx->req.data = work_ctx;
    work_ctx->pipeline = s->pipeline;
    work_ctx->db = c->db;
    char sql_buf[256];
    snprintf(sql_buf, sizeof(sql_buf), "INSERT INTO %s (tag_id, value, ts) VALUES (%u, %f, %lu)",
             c->table, ev->tag_id, ev->payload.val.f, ev->ts);
    work_ctx->sql = strdup(sql_buf);
    uv_queue_work(s->pipeline->engine->loop, &work_ctx->req, _sink_work_cb, _sink_after_work_cb);
    return STAGE_ASYNC;
}

static stage_ret_t _batt_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    battery_limit_t *lm = s->priv_cfg;
    if (ev->payload.val.f > lm->max_discharge_pwr) ev->payload.val.f = lm->max_discharge_pwr;
    if (ev->payload.val.f < -lm->max_charge_pwr) ev->payload.val.f = -lm->max_charge_pwr;
    return STAGE_OK;
}

static stage_ret_t _reflex_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _reflex_cfg_t *c = s->priv_cfg;
    bool triggered = c->greater_than ? (ev->payload.val.f > c->threshold) : (ev->payload.val.f < c->threshold);
    if (triggered) {
        cmd_msg_t cmd = {0};
        cmd.action = CMD_ACTION_WRITE;
        cmd.tag_id = c->target_tag;
        cmd.value = c->action_value;
        cmd.priority = ARBITER_PRIO_HIGH;
        strncpy(cmd.protocol, c->proto, 15);
        snprintf(cmd.req_id, sizeof(cmd.req_id), "REFLEX_%u", c->target_tag);
        bus_orchestrator_submit_cmd(c->orch, (int)c->bus_id, &cmd);
    }
    return STAGE_OK;
}

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

static stage_ret_t _fir_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    dsp_fir_t *fir = (dsp_fir_t *)s->priv_state;
    ev->payload.val.f = dsp_fir_process(fir, ev->payload.val.f);
    return STAGE_OK;
}

static stage_ret_t _kf_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    _kf_st_t *st = s->priv_state;
    control_kf_predict(&st->kf, NULL, NULL, NULL); 
    edge_matrix_t *z = edge_matrix_alloc(st->meas_dim, 1);
    edge_matrix_set(z, 0, 0, ev->payload.val.f);
    control_kf_correct(&st->kf, NULL, z);
    ev->payload.val.f = edge_matrix_get(st->kf.x, 0, 0);
    edge_matrix_free(z);
    return STAGE_OK;
}

static void _kf_cleanup(void *p) {
    _kf_st_t *st = p;
    edge_matrix_free(st->kf.x); edge_matrix_free(st->kf.P);
    edge_matrix_free(st->kf.Q); edge_matrix_free(st->kf.R);
    edge_matrix_free(st->kf.K);
    free(st);
}

static stage_ret_t _foc_proc(pipeline_stage_t *s, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    double v_dc = (double)(uintptr_t)s->priv_cfg;
    if (v_dc <= 0) v_dc = 24.0;
    coord_alphabeta_t v_in = {ev->payload.val.f, 0.0};
    double duties[3];
    motor_svpwm(&v_in, v_dc, duties);
    ev->payload.val.f = duties[0]; 
    return STAGE_OK;
}

// --- V2 Factories (Primary) ---

pipeline_stage_t *stage_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    uint32_t tag_id = (uint32_t)def->params.u1;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter", 31);
    s->process = _filter_proc;
    s->priv_cfg = (void *)(uintptr_t)tag_id;
    return s;
}

pipeline_stage_t *stage_filter_range_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "filter_range", 31);
    s->process = _range_proc;
    _range_cfg_t *c = malloc(sizeof(_range_cfg_t));
    c->min = def->params.f1;
    c->max = def->params.f2;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_transform_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "transform", 31);
    s->process = _trans_proc;
    _trans_cfg_t *c = malloc(sizeof(_trans_cfg_t));
    c->a = def->params.f1;
    c->b = def->params.f2;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_ema_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "ema", 31);
    s->process = _ema_proc;
    _ema_cfg_t *c = malloc(sizeof(_ema_cfg_t));
    c->alpha = def->params.f1;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    s->priv_state = calloc(1, sizeof(_ema_st_t));
    s->cleanup_state = free;
    return s;
}

pipeline_stage_t *stage_clamp_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "clamp", 31);
    s->process = _clamp_proc;
    _clamp_cfg_t *c = malloc(sizeof(_clamp_cfg_t));
    c->min = def->params.f1;
    c->max = def->params.f2;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_peak_shave_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "peak_shave", 31);
    s->process = _shave_proc;
    _shave_cfg_t *c = calloc(1, sizeof(_shave_cfg_t));
    c->algo.upper_threshold = def->params.f1;
    c->algo.max_discharge = 5000.0;
    c->ctrl_tag = (uint32_t)def->params.u1;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_accumulate_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx; (void)def;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "accumulate", 31);
    s->process = _acc_proc;
    s->priv_state = calloc(1, sizeof(double));
    s->cleanup_state = free;
    return s;
}

pipeline_stage_t *stage_freeze_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "freeze", 31);
    s->process = _freeze_proc;
    _freeze_st_t *st = calloc(1, sizeof(_freeze_st_t));
    st->kind = (int)def->params.u1;
    s->priv_state = st;
    s->cleanup_state = free;
    return s;
}

pipeline_stage_t *stage_time_guard_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx; (void)def;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "time_guard", 31);
    s->process = _tg_proc;
    s->priv_state = calloc(1, sizeof(_tg_st_t));
    s->cleanup_state = free;
    return s;
}

static bool _is_safe_table_name(const char *name) {
    if (!name || !*name) return false;
    for (int i = 0; name[i]; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    return true;
}

pipeline_stage_t *stage_sqlite_sink_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    if (!ctx->db || !def->params.s1) return NULL;
    if (!_is_safe_table_name(def->params.s1)) {
        LOG_ERROR(LOG_CAT_CORE, "SQLite Sink: Invalid table name '%s'", def->params.s1);
        return NULL;
    }
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "sqlite_sink", 31);
    s->process = _sink_proc;
    _sink_cfg_t *c = malloc(sizeof(_sink_cfg_t));
    c->db = ctx->db;
    strncpy(c->table, def->params.s1, 63);
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    char sql[256];
    snprintf(sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS %s (tag_id INTEGER, value REAL, ts INTEGER)", c->table);
    sqlite_svc_exec(c->db, sql, NULL, NULL);
    return s;
}

pipeline_stage_t *stage_reflex_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    if (!ctx->orch) return NULL;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "reflex", 31);
    s->process = _reflex_proc;
    _reflex_cfg_t *c = calloc(1, sizeof(_reflex_cfg_t));
    c->orch = ctx->orch;
    c->bus_id = 0;
    if (def->params.s1) strncpy(c->proto, def->params.s1, 15);
    c->target_tag = (uint32_t)def->params.u1;
    c->threshold = def->params.f1;
    c->greater_than = (def->params.f2 > 0.5);
    c->action_value = def->params.f3;
    s->priv_cfg = c;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_moving_average_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    uint32_t window_size = (uint32_t)def->params.u1;
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

pipeline_stage_t *stage_rbe_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "rbe", 31);
    s->process = _rbe_proc;
    _rbe_st_t *st = calloc(1, sizeof(_rbe_st_t));
    st->threshold_pct = def->params.f1;
    s->priv_state = st;
    s->cleanup_state = free;
    return s;
}

pipeline_stage_t *stage_fir_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx; (void)def;
    double dummy_taps[] = {0.2, 0.2, 0.2, 0.2, 0.2};
    dsp_fir_t *fir = dsp_fir_create(dummy_taps, 5);
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "fir_filter", 31);
    s->process = _fir_proc;
    s->priv_state = fir;
    s->cleanup_state = (stage_cleanup_fn)dsp_fir_free;
    return s;
}

pipeline_stage_t *stage_kf_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    uint32_t n = (uint32_t)def->params.u1 ? (uint32_t)def->params.u1 : 1;
    _kf_st_t *st = calloc(1, sizeof(_kf_st_t));
    st->state_dim = n;
    st->meas_dim = 1;
    control_kf_init(&st->kf, n, 1);
    edge_matrix_identity(st->kf.P);
    edge_matrix_identity(st->kf.Q);
    edge_matrix_identity(st->kf.R);
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "kalman_filter", 31);
    s->process = _kf_proc;
    s->priv_state = st;
    s->cleanup_state = _kf_cleanup;
    return s;
}

pipeline_stage_t *stage_foc_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "foc_svpwm", 31);
    s->process = _foc_proc;
    s->priv_cfg = (void *)(uintptr_t)def->params.f1;
    return s;
}

pipeline_stage_t *stage_battery_guard_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    battery_limit_t *limits = calloc(1, sizeof(battery_limit_t));
    limits->max_charge_pwr = (float)def->params.f1;
    limits->max_discharge_pwr = (float)def->params.f2;
    return stage_battery_guard_create(limits);
}

// --- V1 Wrappers (Compatibility) ---

pipeline_stage_t *stage_battery_guard_create(battery_limit_t *limits) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "batt_guard", 31);
    s->process = _batt_proc;
    s->priv_cfg = limits;
    s->cleanup_cfg = free;
    return s;
}

pipeline_stage_t *stage_filter_create(uint32_t tag_id) {
    edge_stage_def_t def = {0}; def.params.u1 = tag_id;
    return stage_filter_create_v2(&def, NULL);
}

pipeline_stage_t *stage_filter_range_create(double min, double max) {
    edge_stage_def_t def = {0}; def.params.f1 = min; def.params.f2 = max;
    return stage_filter_range_create_v2(&def, NULL);
}

pipeline_stage_t *stage_transform_create(double alpha, double beta) {
    edge_stage_def_t def = {0}; def.params.f1 = alpha; def.params.f2 = beta;
    return stage_transform_create_v2(&def, NULL);
}

pipeline_stage_t *stage_ema_create(double alpha) {
    edge_stage_def_t def = {0}; def.params.f1 = alpha;
    return stage_ema_create_v2(&def, NULL);
}

pipeline_stage_t *stage_clamp_create(double min, double max) {
    edge_stage_def_t def = {0}; def.params.f1 = min; def.params.f2 = max;
    return stage_clamp_create_v2(&def, NULL);
}

pipeline_stage_t *stage_peak_shaving_create(double threshold_w, uint32_t ctrl_tag) {
    edge_stage_def_t def = {0}; def.params.f1 = threshold_w; def.params.u1 = ctrl_tag;
    return stage_peak_shave_create_v2(&def, NULL);
}

pipeline_stage_t *stage_accumulate_create(void) {
    edge_stage_def_t def = {0};
    return stage_accumulate_create_v2(&def, NULL);
}

pipeline_stage_t *stage_periodic_freeze_create(int kind) {
    edge_stage_def_t def = {0}; def.params.u1 = (uint32_t)kind;
    return stage_freeze_create_v2(&def, NULL);
}

pipeline_stage_t *stage_time_guard_create(void) {
    edge_stage_def_t def = {0};
    return stage_time_guard_create_v2(&def, NULL);
}

pipeline_stage_t *stage_sqlite_sink_create(sqlite_svc_t *db, const char *table) {
    pipeline_loader_ctx_t ctx = {0}; ctx.db = db;
    edge_stage_def_t def = {0}; def.params.s1 = table;
    return stage_sqlite_sink_create_v2(&def, &ctx);
}

pipeline_stage_t *stage_reflex_logic_create(struct bus_orchestrator_s *orch, uint32_t bus_id, const char *proto, uint32_t target_tag, double threshold, bool greater_than, double action_val) {
    pipeline_loader_ctx_t ctx = {0}; ctx.orch = orch;
    edge_stage_def_t def = {0}; def.params.s1 = proto; def.params.u1 = target_tag; def.params.f1 = threshold; def.params.f2 = greater_than ? 1.0 : 0.0; def.params.f3 = action_val;
    (void)bus_id;
    return stage_reflex_create_v2(&def, &ctx);
}

pipeline_stage_t *stage_moving_average_create(uint32_t window_size) {
    edge_stage_def_t def = {0}; def.params.u1 = window_size;
    return stage_moving_average_create_v2(&def, NULL);
}

pipeline_stage_t *stage_rbe_create(double threshold_pct) {
    edge_stage_def_t def = {0}; def.params.f1 = threshold_pct;
    return stage_rbe_create_v2(&def, NULL);
}

// --- Weak Stubs & Other ---

__attribute__((weak)) pipeline_stage_t *stage_aggregator_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)def; (void)ctx; return NULL;
}

__attribute__((weak)) pipeline_stage_t *stage_linear_system_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)def; (void)ctx; return NULL;
}

pipeline_stage_t *stage_transfer_fn_create(const edge_stage_def_t *def);
__attribute__((weak)) pipeline_stage_t *stage_tf_s2z_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx; return stage_transfer_fn_create(def);
}

pipeline_stage_t *stage_iir_filter_create(uint32_t order, const double *b, const double *a);

pipeline_stage_t *stage_iir_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    // [Fix C6] 移除硬编码，使用传入参数 f1=b0, f2=b1, f3=a1
    uint32_t order = (uint32_t)def->params.u1;
    if (order != 1) return NULL; // 目前仅支持 1 阶通过此接口
    double b[] = {def->params.f1, def->params.f2};
    double a[] = {1.0, def->params.f3};
    return stage_iir_filter_create(order, b, a);
}

pipeline_stage_t *stage_advanced_pid_create(double kp, double ki, double kd, double setpoint, double out_min, double out_max, uint32_t out_tag);

pipeline_stage_t *stage_advanced_pid_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    double setpoint = def->params.s1 ? atof(def->params.s1) : 50.0;
    double out_min = -100.0;
    double out_max = 100.0;
    return stage_advanced_pid_create(def->params.f1, def->params.f2, def->params.f3, setpoint, out_min, out_max, (uint32_t)def->params.u1);
}

#include "autoconf.h"
#include "edge_license.h"

pipeline_stage_t *stage_advanced_pid_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_kf_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_foc_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_fir_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_battery_guard_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);

#if defined(CONFIG_EDGEC_ALGO_LINALG)
pipeline_stage_t *stage_ekf_soc_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_mpc_dispatcher_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_kinematics_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_sensor_fusion_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
#endif

#if defined(CONFIG_EDGEC_ALGO_DSP)
pipeline_stage_t *stage_notch_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_ecg_anomaly_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
#endif

#if defined(CONFIG_EDGEC_ALGO_AHRS)
pipeline_stage_t *stage_ahrs_quaternion_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
#endif

#if defined(CONFIG_EDGEC_ALGO_VIBRA)
pipeline_stage_t *stage_rul_prediction_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_radar_tracker_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_vpp_logger_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_compactor_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_anomaly_baseline_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);

#endif

#if defined(CONFIG_EDGEC_WASM)
pipeline_stage_t *stage_wasm_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
pipeline_stage_t *stage_tinyml_wasm_create(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
#endif

static const edge_stage_factory_entry_t g_standard_stages[] = {
    {"advanced_pid", stage_advanced_pid_create_v2},
#if defined(CONFIG_EDGEC_ALGO_LINALG)
    {"ekf_soc", stage_ekf_soc_create_v2},
    {"mpc_dispatch", stage_mpc_dispatcher_create_v2},
    {"kinematics", stage_kinematics_create_v2},
    {"sensor_fusion", stage_sensor_fusion_create_v2},
#endif
#if defined(CONFIG_EDGEC_ALGO_DSP)
    {"notch_filter", stage_notch_filter_create_v2},
    {"ecg_anomaly", stage_ecg_anomaly_create_v2},
#endif
#if defined(CONFIG_EDGEC_ALGO_AHRS)
    {"ahrs_quaternion", stage_ahrs_quaternion_create_v2},
#endif
#if defined(CONFIG_EDGEC_ALGO_VIBRA)
    {"rul_prediction", stage_rul_prediction_create_v2},
    {"radar_tracker", stage_radar_tracker_create_v2},
    {"vpp_logger", stage_vpp_logger_create_v2},
    {"compactor", stage_compactor_create_v2},
    {"anomaly_baseline", stage_anomaly_baseline_create_v2},
#endif
#if defined(CONFIG_EDGEC_WASM)
    {"wasm", stage_wasm_create_v2},
    {"tinyml_wasm", stage_tinyml_wasm_create},
#endif
    {"kalman", stage_kf_create_v2},
    {"foc", stage_foc_create_v2},
    {"fir_filter", stage_fir_filter_create_v2},
    {"battery_guard", stage_battery_guard_create_v2},
    {"aggregator", stage_aggregator_create_v2},
    {"linear_system", stage_linear_system_create_v2},
    {"tf_s2z", stage_tf_s2z_create_v2},
    {"iir_filter", stage_iir_filter_create_v2},
    {"filter", stage_filter_create_v2},
    {"filter_range", stage_filter_range_create_v2},
    {"transform", stage_transform_create_v2},
    {"ema", stage_ema_create_v2},
    {"clamp", stage_clamp_create_v2},
    {"peak_shave", stage_peak_shave_create_v2},
    {"accumulate", stage_accumulate_create_v2},
    {"freeze", stage_freeze_create_v2},
    {"time_guard", stage_time_guard_create_v2},
    {"sqlite_sink", stage_sqlite_sink_create_v2},
    {"reflex", stage_reflex_create_v2},
    {"moving_average", stage_moving_average_create_v2},
    {"rbe", stage_rbe_create_v2},
    {NULL, NULL}
};

__attribute__((weak)) const edge_stage_factory_entry_t *edge_stage_factory_get_registry(void) {
    return g_standard_stages;
}
