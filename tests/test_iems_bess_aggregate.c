/**
 * @file test_iems_bess_aggregate.c
 * @brief BESS cabinet aggregate host tests (V1-V15, P1-P10).
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iems_bess_aggregate.h"

#define TEST_NOW_NS 1000000000000ULL /* 1000 s, so stale offsets never underflow */
#define TEST_CLOSE 1e-6

typedef struct {
    uint64_t now_ns;
    uint8_t count;
    uint64_t epoch;
    bool bms_read_error;
    uint8_t bms_report_count; /* 0 => report fx->count */

    iems_bess_pcs_status_t pcs[IEMS_BESS_MAX_CABINETS];
    iems_bess_bms_sample_t bms[IEMS_BESS_MAX_CABINETS];

    iems_bess_port_status_t submit_rc[IEMS_BESS_MAX_CABINETS];
    uint32_t submit_calls[IEMS_BESS_MAX_CABINETS];
    uint64_t submit_total;
    double last_target[IEMS_BESS_MAX_CABINETS];
    iems_bess_pcs_mode_t last_mode[IEMS_BESS_MAX_CABINETS];
    double last_reactive[IEMS_BESS_MAX_CABINETS];
    uint32_t last_seq[IEMS_BESS_MAX_CABINETS];
    uint64_t last_issue_ts[IEMS_BESS_MAX_CABINETS];

    uint32_t read_status_calls;
    uint32_t read_all_calls;
} fx_t;

static iems_bess_port_status_t fx_submit(void *ctx, const iems_bess_pcs_command_t *cmd,
                                         iems_bess_pcs_ack_t *ack) {
    fx_t *fx = (fx_t *)ctx;
    assert_true(cmd->cabinet_id >= 1 && cmd->cabinet_id <= IEMS_BESS_MAX_CABINETS);
    uint8_t i = (uint8_t)(cmd->cabinet_id - 1u);
    fx->submit_calls[i]++;
    fx->submit_total++;
    fx->last_target[i] = cmd->active_power_kw;
    fx->last_mode[i] = cmd->mode;
    fx->last_reactive[i] = cmd->reactive_power_kvar;
    fx->last_seq[i] = cmd->command_seq;
    fx->last_issue_ts[i] = cmd->issued_ts_ns;
    ack->status = fx->submit_rc[i];
    ack->accepted_ts_ns = fx->now_ns;
    return fx->submit_rc[i];
}

static iems_bess_port_status_t fx_read_status(void *ctx, uint8_t cabinet_id,
                                              iems_bess_pcs_status_t *out) {
    fx_t *fx = (fx_t *)ctx;
    assert_true(cabinet_id >= 1 && cabinet_id <= IEMS_BESS_MAX_CABINETS);
    fx->read_status_calls++;
    *out = fx->pcs[cabinet_id - 1u];
    return IEMS_BESS_PORT_OK;
}

static iems_bess_port_status_t fx_read_all(void *ctx, iems_bess_bms_sample_t *samples,
                                           uint8_t capacity, uint8_t *out_count,
                                           uint64_t *out_epoch) {
    fx_t *fx = (fx_t *)ctx;
    fx->read_all_calls++;
    if (fx->bms_read_error) return IEMS_BESS_PORT_REJECTED;
    uint8_t n = fx->bms_report_count != 0 ? fx->bms_report_count : fx->count;
    if (n > capacity) n = capacity;
    memcpy(samples, fx->bms, (size_t)n * sizeof(fx->bms[0]));
    *out_count = n;
    *out_epoch = fx->epoch;
    return IEMS_BESS_PORT_OK;
}

static uint64_t fx_now(void *ctx) {
    return ((fx_t *)ctx)->now_ns;
}

static iems_bess_ports_t fx_ports(fx_t *fx) {
    iems_bess_ports_t ports;
    memset(&ports, 0, sizeof(ports));
    ports.pcs.ctx = fx;
    ports.pcs.submit = fx_submit;
    ports.pcs.read_status = fx_read_status;
    ports.bms.ctx = fx;
    ports.bms.read_all = fx_read_all;
    ports.clock.ctx = fx;
    ports.clock.now_ns = fx_now;
    return ports;
}

static void fx_init(fx_t *fx, uint8_t count) {
    memset(fx, 0, sizeof(*fx));
    fx->now_ns = TEST_NOW_NS;
    fx->count = count;
    fx->epoch = 1;
    for (uint8_t i = 0; i < IEMS_BESS_MAX_CABINETS; i++) {
        fx->pcs[i].online = true;
        fx->pcs[i].onoff_acked = true;
        fx->pcs[i].valid = true;
        fx->pcs[i].max_charge_kw = 250.0;
        fx->pcs[i].max_discharge_kw = 250.0;
        fx->pcs[i].sample_ts_ns = fx->now_ns;
        fx->bms[i].online = true;
        fx->bms[i].valid = true;
        fx->bms[i].soc_pct = 50.0;
        fx->bms[i].max_charge_kw = 250.0;
        fx->bms[i].max_discharge_kw = 250.0;
        fx->bms[i].sample_ts_ns = fx->now_ns;
        fx->submit_rc[i] = IEMS_BESS_PORT_OK;
    }
}

static void fx_refresh_ts(fx_t *fx) {
    for (uint8_t i = 0; i < fx->count; i++) {
        fx->pcs[i].sample_ts_ns = fx->now_ns;
        fx->bms[i].sample_ts_ns = fx->now_ns;
    }
}

static void cfg_defaults(iems_bess_config_t *cfg, uint8_t count) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->cabinet_count = count;
    for (uint8_t i = 0; i < count; i++) {
        snprintf(cfg->cabinet[i].lc_no, sizeof(cfg->cabinet[i].lc_no), "LC%02u",
                 (unsigned)(i + 1u));
    }
    cfg->soc_window = (iems_bess_soc_window_t){
        .min = 5.0, .min_back = 10.0, .max_back = 90.0, .max = 95.0};
    cfg->low_soc_threshold_pct = 10.0;
    cfg->low_soc_power_kw = 2.0;
    cfg->low_soc_enable = false;
    cfg->heartbeat_ms = 2000.0;
    cfg->deadband_kw = 0.01;
    cfg->bms_stale_ms = 10000.0;
    cfg->command_timeout_ms = 1000.0;
    cfg->command_fail_limit = 3;
    cfg->policy = IEMS_BESS_POLICY_REDISTRIBUTE;
}

static iems_bess_t *make_bess(iems_bess_storage_t *storage, const iems_bess_config_t *cfg,
                              fx_t *fx) {
    iems_bess_ports_t ports = fx_ports(fx);
    return iems_bess_create_in(storage, cfg, ports);
}

#define assert_close(a, b)                                                                    \
    assert_true(fabs((a) - (b)) <=                                                           \
                TEST_CLOSE * fmax(1.0, fmax(fabs((double)(a)), fabs((double)(b)))))

static void assert_results_same(const iems_bess_result_t *a,
                                const iems_bess_result_t *b) {
    assert_int_equal(a->status, b->status);
    assert_int_equal(a->state, b->state);
    assert_int_equal(a->cabinet_count, b->cabinet_count);
    assert_close(a->requested_kw, b->requested_kw);
    assert_close(a->allocated_kw, b->allocated_kw);
    assert_close(a->shortfall_kw, b->shortfall_kw);
    assert_close(a->reserved_low_soc_kw, b->reserved_low_soc_kw);
    assert_int_equal(a->low_soc_override, b->low_soc_override);
    assert_int_equal(a->input_epoch, b->input_epoch);
    assert_int_equal(a->config_epoch, b->config_epoch);
    for (uint8_t i = 0; i < a->cabinet_count; i++) {
        assert_int_equal(a->cabinet[i].cabinet_id, b->cabinet[i].cabinet_id);
        assert_close(a->cabinet[i].target_kw, b->cabinet[i].target_kw);
        assert_int_equal(a->cabinet[i].state, b->cabinet[i].state);
        assert_int_equal(a->cabinet[i].reason, b->cabinet[i].reason);
        assert_int_equal(a->cabinet[i].submit_status, b->cabinet[i].submit_status);
    }
}

#define request_ok(b, kw, id, r)                                                             \
    do {                                                                                     \
        int rc_ = iems_bess_request_power((b), (kw), (id), (r));                             \
        assert_int_equal(rc_, 0);                                                            \
    } while (0)

static void test_create_validation(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_ports_t ports = fx_ports(&fx);

    assert_true(sizeof(iems_bess_storage_t) == 8192);
    assert_true((uintptr_t)storage.raw % _Alignof(max_align_t) == 0u);

    assert_null(iems_bess_create_in(NULL, &cfg, ports));
    assert_null(iems_bess_create_in(&storage, NULL, ports));

    iems_bess_ports_t bad = ports;
    bad.pcs.submit = NULL;
    assert_null(iems_bess_create_in(&storage, &cfg, bad));
    bad = ports;
    bad.pcs.read_status = NULL;
    assert_null(iems_bess_create_in(&storage, &cfg, bad));
    bad = ports;
    bad.bms.read_all = NULL;
    assert_null(iems_bess_create_in(&storage, &cfg, bad));
    bad = ports;
    bad.clock.now_ns = NULL;
    assert_null(iems_bess_create_in(&storage, &cfg, bad));

    cfg.cabinet_count = 0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.cabinet_count = 21;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.cabinet_count = 2;

    strcpy(cfg.cabinet[1].lc_no, cfg.cabinet[0].lc_no);
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    strcpy(cfg.cabinet[1].lc_no, "LC02");

    cfg.cabinet[0].lc_no[0] = '\0';
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    strcpy(cfg.cabinet[0].lc_no, "LC01");

    memset(cfg.cabinet[0].lc_no, 'A', sizeof(cfg.cabinet[0].lc_no));
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    strcpy(cfg.cabinet[0].lc_no, "LC01");

    iems_bess_soc_window_t good = cfg.soc_window;
    iems_bess_soc_window_t bad_window[6] = {
        {.min = -1.0, .min_back = 10.0, .max_back = 90.0, .max = 95.0},
        {.min = 10.0, .min_back = 10.0, .max_back = 90.0, .max = 95.0},
        {.min = 5.0, .min_back = 20.0, .max_back = 15.0, .max = 95.0},
        {.min = 5.0, .min_back = 10.0, .max_back = 95.0, .max = 95.0},
        {.min = 5.0, .min_back = 10.0, .max_back = 90.0, .max = 101.0},
        {.min = NAN, .min_back = 10.0, .max_back = 90.0, .max = 95.0},
    };
    for (size_t i = 0; i < sizeof(bad_window) / sizeof(bad_window[0]); i++) {
        cfg.soc_window = bad_window[i];
        assert_null(iems_bess_create_in(&storage, &cfg, ports));
    }
    cfg.soc_window = good;

    cfg.low_soc_power_kw = -1.0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.low_soc_power_kw = NAN;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.low_soc_power_kw = 2.0;
    cfg.low_soc_threshold_pct = 101.0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.low_soc_threshold_pct = 10.0;

    cfg.heartbeat_ms = 0.0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.heartbeat_ms = 2000.0;
    cfg.deadband_kw = -0.01;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.deadband_kw = 0.01;
    cfg.bms_stale_ms = 0.0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.bms_stale_ms = 10000.0;
    cfg.command_timeout_ms = 0.0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.command_timeout_ms = 1000.0;
    cfg.command_fail_limit = 0;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.command_fail_limit = 3;
    cfg.policy = (iems_bess_alloc_policy_t)99;
    assert_null(iems_bess_create_in(&storage, &cfg, ports));
    cfg.policy = IEMS_BESS_POLICY_REDISTRIBUTE;

    iems_bess_t *bess = iems_bess_create_in(&storage, &cfg, ports);
    assert_non_null(bess);
    iems_bess_result_t r;
    memset(&r, 0xA5, sizeof(r));
    iems_bess_result_t untouched = r;
    assert_int_equal(iems_bess_get_last_result(bess, &r), IEMS_BESS_INVALID);
    assert_memory_equal(&r, &untouched, sizeof(r));
    assert_int_equal(iems_bess_get_last_result(bess, NULL), IEMS_BESS_INVALID);

    iems_bess_destroy(bess);
    iems_bess_destroy(NULL);
    iems_bess_destroy(bess);
    assert_int_equal(iems_bess_request_power(bess, 0.0, 1, &r), IEMS_BESS_INVALID);
    bess = iems_bess_create_in(&storage, &cfg, ports);
    assert_non_null(bess);
    iems_bess_destroy(bess);

    cfg_defaults(&cfg, 20);
    bess = iems_bess_create_in(&storage, &cfg, ports);
    assert_non_null(bess);
    iems_bess_destroy(bess);
}

static void test_value_updates(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 1);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 1);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    assert_non_null(bess);

    iems_bess_soc_window_t w = {.min = 10.0, .min_back = 20.0, .max_back = 80.0, .max = 90.0};
    assert_int_equal(iems_bess_set_soc_window(bess, &w), 0);
    iems_bess_soc_window_t bad = {.min = 20.0, .min_back = 10.0, .max_back = 80.0, .max = 90.0};
    assert_int_equal(iems_bess_set_soc_window(bess, &bad), IEMS_BESS_INVALID);
    assert_int_equal(iems_bess_set_soc_window(bess, NULL), IEMS_BESS_INVALID);

    assert_int_equal(iems_bess_set_low_soc(bess, true, 15.0, 3.0), 0);
    assert_int_equal(iems_bess_set_low_soc(bess, true, -1.0, 3.0), IEMS_BESS_INVALID);
    assert_int_equal(iems_bess_set_low_soc(bess, true, 15.0, -3.0), IEMS_BESS_INVALID);
    assert_int_equal(iems_bess_set_low_soc(bess, true, NAN, 3.0), IEMS_BESS_INVALID);

    iems_bess_result_t r;
    iems_bess_tick(bess, &r);
    assert_int_equal(r.config_epoch, 2u); /* two accepted config updates */
    iems_bess_destroy(bess);
}

static void test_site_fault_v13(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(fx.submit_total, 2u);
    assert_close(r.allocated_kw, 100.0);
    assert_int_equal(r.state, IEMS_BESS_STATE_DISPATCHING);

    assert_int_equal(iems_bess_set_site_fault(bess, IEMS_BESS_SITE_FAULT_FIRE), 0);
    assert_int_equal(fx.submit_total, 4u); /* immediate safety zero */
    iems_bess_result_t sentinel;
    memset(&sentinel, 0x5A, sizeof(sentinel));
    iems_bess_result_t out = sentinel;
    assert_int_equal(iems_bess_request_power(bess, 100.0, 2, &out), IEMS_BESS_TRIPPED);
    assert_int_not_equal(out.status, sentinel.status);
    assert_int_equal(out.status, IEMS_BESS_TRIPPED);
    assert_int_equal(out.state, IEMS_BESS_STATE_TRIPPED);
    assert_close(out.allocated_kw, 0.0);
    for (uint8_t i = 0; i < 2; i++) {
        assert_close(out.cabinet[i].target_kw, 0.0);
        assert_int_equal(out.cabinet[i].state, IEMS_BESS_CAB_FAULT);
        assert_int_equal(out.cabinet[i].reason, IEMS_BESS_REASON_TRIPPED);
    }
    assert_int_equal(iems_bess_reset_site_faults(bess), IEMS_BESS_TRIPPED);
    assert_int_equal(iems_bess_set_site_fault(bess, 0u), 0);
    assert_int_equal(iems_bess_reset_site_faults(bess), 0);
    request_ok(bess, 100.0, 3, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_close(r.cabinet[0].target_kw, 50.0);

    assert_int_equal(iems_bess_set_site_fault(bess, IEMS_BESS_SITE_FAULT_WATCHDOG), 0);
    assert_int_equal(iems_bess_request_power(bess, 100.0, 4, &r), IEMS_BESS_WATCHDOG);
    assert_int_equal(r.status, IEMS_BESS_WATCHDOG);
    assert_int_equal(iems_bess_set_site_fault(bess, 0u), 0);
    assert_int_equal(iems_bess_reset_site_faults(bess), 0);
    iems_bess_destroy(bess);
}

static void test_cabinet_faults_v6(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.pcs[0].fault_bits = 0x3u;
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_FAULT);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_close(r.cabinet[1].target_kw, 100.0);
    assert_close(r.allocated_kw, 100.0);

    fx.pcs[0].fault_bits = 0x1u; /* only one of the two sources clears */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_FAULT);
    assert_close(r.cabinet[0].target_kw, 0.0);

    fx.pcs[0].fault_bits = 0u; /* active clear, latch remains */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_FAULT_LATCHED);
    assert_close(r.cabinet[0].target_kw, 0.0);

    assert_int_equal(iems_bess_reset_faults(bess, 1), 0);
    assert_int_equal(iems_bess_reset_faults(bess, 0), IEMS_BESS_INVALID);
    assert_int_equal(iems_bess_reset_faults(bess, 3), IEMS_BESS_INVALID);
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);
    assert_close(r.cabinet[0].target_kw, 50.0);
    iems_bess_destroy(bess);
}

static void test_availability_arbitration_v7(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.bms[0].online = false;
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_OFFLINE);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_OFFLINE);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_close(r.cabinet[1].target_kw, 100.0);

    fx.bms[0].online = true;
    fx.pcs[0].online = false;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_OFFLINE);

    fx.pcs[0].online = true;
    fx.pcs[0].valid = false;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_STALE);

    fx.pcs[0].valid = true;
    fx.pcs[0].onoff_acked = false;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_OFF);

    fx.pcs[0].onoff_acked = true;
    fx.now_ns += 20000000000ULL; /* 20 s > bms_stale_ms */
    assert_int_equal(iems_bess_tick(bess, &r), IEMS_BESS_NO_DATA);
    assert_int_equal(r.status, IEMS_BESS_NO_DATA);
    assert_int_equal(r.state, IEMS_BESS_STATE_INIT);
    for (uint8_t i = 0; i < 2; i++) {
        assert_int_equal(r.cabinet[i].state, IEMS_BESS_CAB_STALE);
        assert_close(r.cabinet[i].target_kw, 0.0);
    }

    fx.pcs[0].sample_ts_ns = fx.now_ns;
    fx.bms[0].sample_ts_ns = fx.now_ns;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);
    assert_int_equal(r.cabinet[1].state, IEMS_BESS_CAB_STALE);
    assert_close(r.cabinet[0].target_kw, 100.0);
    iems_bess_destroy(bess);
}

/* The BMS snapshot is all-or-nothing: a port reject or a count mismatch fails
 * safe with NO_DATA instead of mixing stale cabinet slots into a plan. */
static void test_bms_port_failure_is_safe(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_close(fx.last_target[0], 50.0);

    fx.bms_read_error = true;
    assert_int_equal(iems_bess_tick(bess, &r), IEMS_BESS_NO_DATA);
    assert_int_equal(r.state, IEMS_BESS_STATE_INIT);
    for (uint8_t i = 0; i < 2; i++) {
        assert_int_equal(r.cabinet[i].state, IEMS_BESS_CAB_STALE);
        assert_close(r.cabinet[i].target_kw, 0.0);
        assert_close(fx.last_target[i], 0.0);
    }

    fx.bms_read_error = false;
    fx.bms_report_count = 1;
    assert_int_equal(iems_bess_tick(bess, &r), IEMS_BESS_NO_DATA);

    fx.bms_report_count = 0;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.state, IEMS_BESS_STATE_DISPATCHING);
    assert_close(r.allocated_kw, 100.0);
    iems_bess_destroy(bess);
}

static void test_onoff_connect(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_close(fx.last_target[0], 50.0);

    assert_int_equal(iems_bess_set_onoff(bess, false, 2, &r), 0);
    assert_int_equal(r.state, IEMS_BESS_STATE_READY);
    for (uint8_t i = 0; i < 2; i++) {
        assert_int_equal(r.cabinet[i].state, IEMS_BESS_CAB_OFF);
        assert_close(r.cabinet[i].target_kw, 0.0);
        assert_close(fx.last_target[i], 0.0);
        assert_int_equal(fx.last_mode[i], IEMS_BESS_PCS_MODE_STANDBY);
    }
    assert_int_equal(iems_bess_set_onoff(bess, true, 3, &r), 0);
    assert_close(r.cabinet[0].target_kw, 50.0);
    assert_int_equal(fx.last_mode[0], IEMS_BESS_PCS_MODE_PQ);

    assert_int_equal(iems_bess_set_connect(bess, false, 4, &r), 0);
    for (uint8_t i = 0; i < 2; i++) {
        assert_close(r.cabinet[i].target_kw, 0.0);
        assert_int_equal(fx.last_mode[i], IEMS_BESS_PCS_MODE_STANDBY);
    }
    assert_int_equal(iems_bess_set_connect(bess, true, 5, &r), 0);
    assert_close(r.cabinet[0].target_kw, 50.0);
    iems_bess_destroy(bess);
}

static void test_soc_hysteresis(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 1);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 1);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.bms[0].soc_pct = 96.0;
    request_ok(bess, -10.0, 1, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_CHARGE);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_SOC_FULL);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_int_equal(r.status, IEMS_BESS_NO_ELIGIBLE);

    fx.bms[0].soc_pct = 91.0; /* still >= max_back: stays FULL */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_CHARGE);

    fx.bms[0].soc_pct = 89.0; /* < max_back: back to OR */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);
    assert_close(r.cabinet[0].target_kw, -10.0);

    fx.bms[0].soc_pct = 4.0;
    request_ok(bess, 10.0, 2, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_DISCHARGE);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_SOC_EMPTY);
    assert_int_equal(r.status, IEMS_BESS_NO_ELIGIBLE);

    fx.bms[0].soc_pct = 9.0; /* still <= min_back: stays EMPTY */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_DISCHARGE);

    fx.bms[0].soc_pct = 11.0; /* > min_back: back to OR */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);
    assert_close(r.cabinet[0].target_kw, 10.0);
    iems_bess_destroy(bess);
}

static void test_low_soc_threshold_v8(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 1);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 1);
    cfg.low_soc_enable = true;
    cfg.low_soc_threshold_pct = 10.5;
    cfg.low_soc_power_kw = 2.0;
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.bms[0].soc_pct = 10.2; /* v3 truncated the threshold to 10 and missed this */
    request_ok(bess, 0.0, 1, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_LOW_SOC);
    assert_close(r.cabinet[0].target_kw, -2.0);
    assert_close(r.reserved_low_soc_kw, 2.0);
    assert_true(r.low_soc_override);
    assert_close(r.allocated_kw, -2.0);

    fx.bms[0].soc_pct = 10.6;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_false(r.low_soc_override);
    iems_bess_destroy(bess);
}

static void test_v1_v2_v3(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 10);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 10);
    fx.pcs[0].max_discharge_kw = 2.0;
    iems_bess_storage_t storage;
    iems_bess_result_t r;

    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_close(r.allocated_kw, 100.0);
    assert_close(r.shortfall_kw, 0.0);
    assert_close(r.cabinet[0].target_kw, 2.0);
    for (uint8_t i = 1; i < 10; i++) {
        assert_close(r.cabinet[i].target_kw, 98.0 / 9.0);
    }
    iems_bess_destroy(bess);

    cfg.policy = IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER;
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_SHORTFALL);
    assert_close(r.allocated_kw, 92.0);
    assert_close(r.shortfall_kw, 8.0);
    assert_close(r.cabinet[0].target_kw, 2.0);
    for (uint8_t i = 1; i < 10; i++) {
        assert_close(r.cabinet[i].target_kw, 10.0);
    }
    iems_bess_destroy(bess);

    fx_init(&fx, 2);
    cfg_defaults(&cfg, 2);
    fx.pcs[0].max_discharge_kw = 30.0;
    fx.pcs[1].max_discharge_kw = 30.0;
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_SHORTFALL);
    assert_close(r.cabinet[0].target_kw, 30.0);
    assert_close(r.cabinet[1].target_kw, 30.0);
    assert_close(r.allocated_kw, 60.0);
    assert_close(r.shortfall_kw, 40.0);
    iems_bess_destroy(bess);

    fx_init(&fx, 4);
    cfg_defaults(&cfg, 4);
    for (uint8_t i = 0; i < 4; i++) {
        fx.pcs[i].online = false;
        fx.bms[i].online = false;
    }
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 50.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_NO_ELIGIBLE);
    assert_int_equal(r.state, IEMS_BESS_STATE_DEGRADED);
    assert_close(r.allocated_kw, 0.0);
    assert_close(r.shortfall_kw, 50.0);
    for (uint8_t i = 0; i < 4; i++) {
        assert_close(r.cabinet[i].target_kw, 0.0);
        assert_int_equal(r.cabinet[i].state, IEMS_BESS_CAB_OFFLINE);
    }
    iems_bess_destroy(bess);

    fx_init(&fx, 2);
    cfg_defaults(&cfg, 2);
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(fx.submit_total, 2u);
    request_ok(bess, 0.0, 2, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_int_equal(r.state, IEMS_BESS_STATE_READY);
    assert_close(r.allocated_kw, 0.0);
    assert_close(fx.last_target[0], 0.0);
    assert_close(fx.last_target[1], 0.0);
    assert_int_equal(fx.submit_total, 4u);
    iems_bess_destroy(bess);
}

static void test_v4_v5(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 5);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 5);
    fx.bms[0].soc_pct = 100.0;
    iems_bess_storage_t storage;
    iems_bess_result_t r;

    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, -50.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_CHARGE);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_SOC_FULL);
    for (uint8_t i = 1; i < 5; i++) {
        assert_close(r.cabinet[i].target_kw, -12.5);
    }
    assert_close(r.allocated_kw, -50.0);
    iems_bess_destroy(bess);

    cfg.policy = IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER;
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, -50.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    for (uint8_t i = 1; i < 5; i++) {
        assert_close(r.cabinet[i].target_kw, -12.5);
    }
    assert_close(r.allocated_kw, -50.0);
    assert_close(r.shortfall_kw, 0.0);
    iems_bess_destroy(bess);

    fx_init(&fx, 5);
    cfg_defaults(&cfg, 5);
    cfg.low_soc_enable = true;
    cfg.low_soc_threshold_pct = 10.0;
    cfg.low_soc_power_kw = 2.0;
    fx.bms[0].soc_pct = 8.0;

    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, -50.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_LOW_SOC);
    assert_close(r.cabinet[0].target_kw, -2.0);
    for (uint8_t i = 1; i < 5; i++) {
        assert_close(r.cabinet[i].target_kw, -12.0);
    }
    assert_close(r.allocated_kw, -50.0);
    assert_close(r.reserved_low_soc_kw, 2.0);
    assert_false(r.low_soc_override);
    iems_bess_destroy(bess);

    cfg.policy = IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER;
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, -50.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_SHORTFALL);
    assert_close(r.cabinet[0].target_kw, -2.0);
    for (uint8_t i = 1; i < 5; i++) {
        assert_close(r.cabinet[i].target_kw, -10.0);
    }
    assert_close(r.allocated_kw, -42.0);
    assert_close(r.reserved_low_soc_kw, 2.0);
    assert_close(r.shortfall_kw, 8.0);
    assert_false(r.low_soc_override);

    /* Low SOC reserve outranks the requested direction under REDISTRIBUTE. */
    fx.bms[0].soc_pct = 8.0;
    cfg.policy = IEMS_BESS_POLICY_REDISTRIBUTE;
    iems_bess_destroy(bess);
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 50.0, 1, &r);
    assert_true(r.low_soc_override);
    assert_close(r.allocated_kw, -2.0);
    assert_close(r.reserved_low_soc_kw, 2.0);
    assert_close(r.shortfall_kw, 0.0);
    for (uint8_t i = 1; i < 5; i++) {
        assert_close(r.cabinet[i].target_kw, 0.0);
    }
    iems_bess_destroy(bess);
}

static void test_direction_blocked_sends_zero_v10(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_close(fx.last_target[0], 50.0);

    fx.bms[0].soc_pct = 100.0;
    fx.bms[1].soc_pct = 100.0;
    request_ok(bess, -100.0, 2, &r);
    assert_int_equal(r.status, IEMS_BESS_NO_ELIGIBLE);
    assert_close(r.allocated_kw, 0.0);
    assert_close(r.shortfall_kw, 100.0);
    for (uint8_t i = 0; i < 2; i++) {
        assert_int_equal(r.cabinet[i].state, IEMS_BESS_CAB_BLOCKED_CHARGE);
        assert_close(r.cabinet[i].target_kw, 0.0);
        assert_close(fx.last_target[i], 0.0);
    }
    assert_int_equal(fx.submit_total, 4u);
    iems_bess_destroy(bess);
}

static void test_idempotency_v11(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t first;
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &first);
    assert_int_equal(first.state, IEMS_BESS_STATE_DISPATCHING);
    assert_int_equal(fx.submit_total, 2u);
    for (int i = 0; i < 9; i++) {
        request_ok(bess, 100.0, 1, &r);
        assert_results_same(&first, &r);
    }
    assert_int_equal(fx.submit_total, 2u);
    assert_int_equal(fx.read_all_calls, 1u);
    assert_int_equal(fx.read_status_calls, 2u);

    request_ok(bess, 100.0, 2, &r); /* new id, same plan: dedup, no device write */
    assert_int_equal(fx.submit_total, 2u);

    iems_bess_result_t sentinel;
    memset(&sentinel, 0xA5, sizeof(sentinel));
    iems_bess_result_t out = sentinel;
    assert_int_equal(iems_bess_request_power(bess, 100.0, 1, &out), IEMS_BESS_STALE_REQUEST);
    assert_memory_equal(&out, &sentinel, sizeof(out));
    out = sentinel;
    assert_int_equal(iems_bess_request_power(bess, 200.0, 2, &out), IEMS_BESS_INVALID);
    assert_memory_equal(&out, &sentinel, sizeof(out));
    out = sentinel;
    assert_int_equal(iems_bess_request_power(bess, NAN, 3, &out), IEMS_BESS_INVALID);
    assert_memory_equal(&out, &sentinel, sizeof(out));
    assert_int_equal(fx.submit_total, 2u);

    iems_bess_result_t last;
    assert_int_equal(iems_bess_get_last_result(bess, &last), 0);
    assert_close(last.requested_kw, 100.0);
    iems_bess_destroy(bess);

    /* request_id 0 is a valid first command. */
    fx_init(&fx, 1);
    cfg_defaults(&cfg, 1);
    bess = make_bess(&storage, &cfg, &fx);
    request_ok(bess, 10.0, 0, &r);
    assert_close(r.allocated_kw, 10.0);
    iems_bess_destroy(bess);
}

static void test_deadband_and_heartbeat_v14(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    cfg.deadband_kw = 0.01;
    cfg.heartbeat_ms = 2000.0;
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(fx.submit_total, 2u);

    fx.now_ns += 1000000ULL; /* 1 ms */
    request_ok(bess, 100.005, 2, &r); /* per-cabinet delta 0.0025 <= 0.01 */
    assert_close(r.allocated_kw, 100.005);
    assert_int_equal(fx.submit_total, 2u);

    fx.now_ns += 1000000ULL;
    request_ok(bess, 100.04, 3, &r); /* per-cabinet delta 0.02 > 0.01 */
    assert_int_equal(fx.submit_total, 4u);
    uint64_t last_submit_ns = fx.now_ns;

    fx.now_ns = last_submit_ns + 1999000000ULL; /* 1999 ms later */
    request_ok(bess, 100.04, 4, &r);
    assert_int_equal(fx.submit_total, 4u);

    fx.now_ns = last_submit_ns + 2000000000ULL; /* heartbeat expires */
    request_ok(bess, 100.04, 5, &r);
    assert_int_equal(fx.submit_total, 6u);
    for (uint8_t i = 0; i < 2; i++) {
        assert_close(fx.last_target[i], 50.02);
        assert_int_equal(fx.last_seq[i], r.command_seq);
        assert_int_equal(fx.last_issue_ts[i], fx.now_ns);
    }
    iems_bess_destroy(bess);
}

static void test_clock_regression_is_safe(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    uint64_t total = fx.submit_total;
    fx.now_ns -= 1000000ULL; /* clock steps backwards: the snapshots read as future */
    assert_int_equal(iems_bess_request_power(bess, 100.0, 2, &r), IEMS_BESS_NO_DATA);
    assert_close(r.allocated_kw, 0.0);
    assert_int_equal(fx.submit_total, total + 2u); /* fail-safe zero, no negative timeout */
    total = fx.submit_total;

    fx.now_ns += 3000000000ULL;
    request_ok(bess, 100.0, 3, &r);
    assert_int_equal(fx.submit_total, total + 2u);
    assert_close(r.allocated_kw, 100.0);
    iems_bess_destroy(bess);
}

static void test_submit_failures_v12(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    cfg.command_fail_limit = 3;
    cfg.command_timeout_ms = 1000.0;
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.submit_rc[0] = IEMS_BESS_PORT_REJECTED;
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.status, IEMS_BESS_PARTIAL);
    assert_int_equal(r.state, IEMS_BESS_STATE_DEGRADED);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_COMMAND_FAILED);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_CMD_FAILED);
    assert_int_equal(r.cabinet[0].submit_status, IEMS_BESS_PORT_REJECTED);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_close(r.cabinet[1].target_kw, 50.0);
    assert_close(r.allocated_kw, 50.0);

    fx.now_ns += 1000000000ULL;
    fx_refresh_ts(&fx);
    request_ok(bess, 100.0, 2, &r);
    assert_int_equal(r.status, IEMS_BESS_PARTIAL);
    fx.now_ns += 1000000000ULL;
    fx_refresh_ts(&fx);
    request_ok(bess, 100.0, 3, &r);
    assert_int_equal(r.status, IEMS_BESS_PARTIAL);

    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_FAULT_LATCHED);
    assert_close(r.cabinet[0].target_kw, 0.0);

    assert_int_equal(iems_bess_reset_faults(bess, 1), 0);
    fx.submit_rc[0] = IEMS_BESS_PORT_OK;
    fx.now_ns += 1000000000ULL;
    fx_refresh_ts(&fx);
    request_ok(bess, 100.0, 4, &r);
    assert_int_equal(r.status, IEMS_BESS_OK);
    assert_close(r.cabinet[0].target_kw, 50.0);

    fx.submit_rc[1] = IEMS_BESS_PORT_TIMEOUT;
    fx.now_ns += 2000000000ULL;
    fx_refresh_ts(&fx);
    request_ok(bess, 100.0, 5, &r);
    assert_int_equal(r.status, IEMS_BESS_PARTIAL);
    assert_int_equal(r.cabinet[1].submit_status, IEMS_BESS_PORT_TIMEOUT);
    assert_close(r.cabinet[1].target_kw, 0.0);
    iems_bess_destroy(bess);
}

static void test_stale_snapshot_and_old_sample_v7_p8(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    cfg.bms_stale_ms = 5000.0;
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &r);
    assert_close(r.cabinet[0].target_kw, 50.0);

    /* An older BMS sample must not overwrite the stored newer one. */
    uint64_t newer_ts = fx.bms[0].sample_ts_ns;
    fx.bms[0].max_discharge_kw = 0.0;
    fx.bms[0].sample_ts_ns = newer_ts - 1000000000ULL;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_close(r.cabinet[0].target_kw, 50.0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_READY);

    /* An older PCS sample is dropped the same way. */
    fx.pcs[0].max_discharge_kw = 0.0;
    fx.pcs[0].sample_ts_ns = newer_ts - 1000000000ULL;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_close(r.cabinet[0].target_kw, 50.0);

    /* BMS offline excludes the cabinet immediately, next tick. */
    fx.bms[0].online = false;
    fx.bms[0].sample_ts_ns = newer_ts;
    fx.pcs[0].sample_ts_ns = newer_ts;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_OFFLINE);
    assert_close(r.cabinet[1].target_kw, 100.0);

    /* Expiry beyond bms_stale_ms turns the cabinet STALE, no guessing. */
    fx.bms[0].online = true;
    fx.now_ns += 6000000000ULL;
    assert_int_equal(iems_bess_tick(bess, &r), IEMS_BESS_NO_DATA);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_STALE);
    iems_bess_destroy(bess);
}

static void test_invalid_samples_are_finite(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    fx.bms[0].soc_pct = NAN;
    request_ok(bess, 100.0, 1, &r);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_STALE);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_close(r.cabinet[1].target_kw, 100.0);
    assert_true(isfinite(r.allocated_kw) && isfinite(r.shortfall_kw));

    fx.bms[0].soc_pct = 50.0;
    fx.bms[0].max_discharge_kw = -10.0; /* negative limit normalizes to 0 */
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_BLOCKED_DISCHARGE);
    assert_int_equal(r.cabinet[0].reason, IEMS_BESS_REASON_LIMIT);
    assert_close(r.cabinet[0].target_kw, 0.0);
    assert_close(r.cabinet[1].target_kw, 100.0);

    fx.bms[0].max_discharge_kw = 250.0;
    fx.pcs[0].measured_active_kw = NAN;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_STALE);
    assert_close(r.cabinet[1].target_kw, 100.0);

    fx.pcs[0].measured_active_kw = INFINITY;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.cabinet[0].state, IEMS_BESS_CAB_STALE);

    fx.pcs[0].measured_active_kw = 10.9; /* double is not truncated to int */
    int rc = iems_bess_request_power(bess, 100.0, 2, &r);
    assert_int_equal(rc, 0);
    assert_close(r.allocated_kw, 100.0);
    iems_bess_destroy(bess);
}

static void test_tick_epoch_and_cache(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 2);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 2);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t first;
    iems_bess_result_t r;

    request_ok(bess, 100.0, 1, &first);
    uint32_t seq = first.command_seq;

    fx.epoch = 7;
    fx.bms[0].max_discharge_kw = 10.0;
    assert_int_equal(iems_bess_tick(bess, &r), 0);
    assert_int_equal(r.input_epoch, 7u);
    assert_close(r.cabinet[0].target_kw, 10.0);
    assert_close(r.cabinet[1].target_kw, 90.0);
    assert_true(r.command_seq > seq);

    /* Duplicate command returns the original cache, not the tick recompute. */
    request_ok(bess, 100.0, 1, &r);
    assert_results_same(&first, &r);
    iems_bess_destroy(bess);
}

static void test_reactive_never_implicit_v15(void **state) {
    (void)state;
    fx_t fx;
    fx_init(&fx, 3);
    iems_bess_config_t cfg;
    cfg_defaults(&cfg, 3);
    iems_bess_storage_t storage;
    iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
    iems_bess_result_t r;

    request_ok(bess, 60.0, 1, &r);
    request_ok(bess, -90.0, 2, &r);
    iems_bess_tick(bess, &r);
    for (uint8_t i = 0; i < 3; i++) {
        assert_close(fx.last_reactive[i], 0.0);
    }
    iems_bess_destroy(bess);
}

/* ------------------------------------------------------------------------- */
/* Property tests (P1-P10), fixed seed                                       */
/* ------------------------------------------------------------------------- */

static uint64_t g_rng_state;

static uint64_t rnd64(void) {
    uint64_t x = g_rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    g_rng_state = x;
    return x;
}

static uint32_t rnd_u32(uint32_t bound) {
    return (uint32_t)(rnd64() % bound);
}

static double rnd_double(double lo, double hi) {
    double u = (double)(rnd64() >> 11) * (1.0 / 9007199254740992.0);
    return lo + (hi - lo) * u;
}

static double effective_cap(double limit_bms, double limit_pcs, bool zero) {
    if (zero) return 0.0;
    double cap = fmin(limit_bms, limit_pcs);
    return cap > 0.0 ? cap : 0.0;
}

static void test_properties_10k(void **state) {
    (void)state;
    g_rng_state = 0x9E3779B97F4A7C15ULL;
    for (uint32_t iteration = 0; iteration < 10000u; iteration++) {
        fx_t fx;
        uint8_t n = (uint8_t)(1u + rnd_u32(IEMS_BESS_MAX_CABINETS));
        fx_init(&fx, n);
        iems_bess_config_t cfg;
        cfg_defaults(&cfg, n);
        cfg.policy = rnd_u32(2u) == 0u ? IEMS_BESS_POLICY_REDISTRIBUTE
                                       : IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER;
        cfg.low_soc_enable = rnd_u32(2u) == 0u;
        cfg.low_soc_threshold_pct = rnd_double(0.0, 30.0);
        cfg.low_soc_power_kw = rnd_double(0.0, 30.0);

        for (uint8_t i = 0; i < n; i++) {
            fx.pcs[i].online = rnd_u32(10u) != 0u;
            fx.bms[i].online = rnd_u32(10u) != 0u;
            fx.pcs[i].valid = rnd_u32(20u) != 0u;
            fx.bms[i].valid = rnd_u32(20u) != 0u;
            fx.pcs[i].onoff_acked = rnd_u32(10u) != 0u;
            fx.pcs[i].max_charge_kw = rnd_u32(10u) == 0u ? -rnd_double(0.0, 20.0)
                                                        : rnd_double(0.0, 250.0);
            fx.pcs[i].max_discharge_kw = rnd_u32(10u) == 0u ? -rnd_double(0.0, 20.0)
                                                           : rnd_double(0.0, 250.0);
            fx.bms[i].max_charge_kw = rnd_u32(10u) == 0u ? -rnd_double(0.0, 20.0)
                                                        : rnd_double(0.0, 250.0);
            fx.bms[i].max_discharge_kw = rnd_u32(10u) == 0u ? -rnd_double(0.0, 20.0)
                                                           : rnd_double(0.0, 250.0);
            fx.bms[i].soc_pct = rnd_double(0.0, 100.0);
            fx.pcs[i].fault_bits = rnd_u32(8u) == 0u ? (1u << rnd_u32(11u)) : 0u;
            fx.bms[i].fault_bits = rnd_u32(8u) == 0u ? (1u << rnd_u32(11u)) : 0u;
            fx.pcs[i].sample_ts_ns = fx.now_ns;
            fx.bms[i].sample_ts_ns = fx.now_ns;
            if (rnd_u32(20u) == 0u) {
                fx.pcs[i].sample_ts_ns -= 20000000000ULL; /* stale */
            }
        }

        double requested = rnd_double(-800.0, 800.0);
        if (rnd_u32(10u) == 0u) requested = rnd_double(-5000.0, 5000.0);

        iems_bess_storage_t storage;
        iems_bess_t *bess = make_bess(&storage, &cfg, &fx);
        assert_non_null(bess);
        iems_bess_result_t r1;
        iems_bess_result_t r2;
        int rc1 = iems_bess_request_power(bess, requested, 1, &r1);
        int rc2 = iems_bess_request_power(bess, requested, 2, &r2);

        assert_int_equal(rc1, rc2);
        assert_int_equal(r1.cabinet_count, n);
        assert_true(isfinite(r1.requested_kw) && isfinite(r1.allocated_kw));
        assert_true(isfinite(r1.shortfall_kw) && isfinite(r1.reserved_low_soc_kw));
        assert_true(r1.shortfall_kw >= 0.0);
        assert_true(r1.reserved_low_soc_kw >= 0.0);

        /* P1: per-cabinet target stays inside the direction cap. */
        for (uint8_t i = 0; i < n; i++) {
            const iems_bess_cab_result_t *cab = &r1.cabinet[i];
            assert_true(isfinite(cab->target_kw));
            bool zero_charge = fx.bms[i].soc_pct >= cfg.soc_window.max;
            bool zero_discharge = fx.bms[i].soc_pct <= cfg.soc_window.min;
            double cap_c = effective_cap(fx.bms[i].max_charge_kw, fx.pcs[i].max_charge_kw,
                                         zero_charge);
            double cap_d = effective_cap(fx.bms[i].max_discharge_kw,
                                         fx.pcs[i].max_discharge_kw, zero_discharge);
            if (cab->target_kw > 0.0) {
                assert_true(cab->target_kw <= cap_d + TEST_CLOSE);
            } else if (cab->target_kw < 0.0) {
                assert_true(-cab->target_kw <= cap_c + TEST_CLOSE);
            }
        }

        /* P2: total ledger, low SOC reserve accounted through the override flag. */
        if ((r1.status == IEMS_BESS_OK || r1.status == IEMS_BESS_SHORTFALL ||
             r1.status == IEMS_BESS_NO_ELIGIBLE) &&
            !r1.low_soc_override) {
            if (cfg.policy == IEMS_BESS_POLICY_REDISTRIBUTE) {
                assert_close(fabs(r1.requested_kw),
                             fabs(r1.allocated_kw) + r1.shortfall_kw);
            } else {
                double same = 0.0;
                for (uint8_t i = 0; i < n; i++) {
                    if (requested < 0.0 && r1.cabinet[i].target_kw < 0.0) {
                        same += -r1.cabinet[i].target_kw;
                    } else if (requested > 0.0 && r1.cabinet[i].target_kw > 0.0) {
                        same += r1.cabinet[i].target_kw;
                    }
                }
                assert_close(fabs(requested), same + r1.shortfall_kw);
            }
        }

        /* P3: identical input and snapshot give an identical result. */
        assert_results_same(&r1, &r2);

        /* P5: repeating the same request_id does not touch a port. */
        uint64_t total_before = fx.submit_total;
        iems_bess_result_t cached;
        int rc_repeat = iems_bess_request_power(bess, requested, 2, &cached);
        if (rc2 >= 0) {
            assert_int_equal(rc_repeat, 0);
            assert_results_same(&r2, &cached);
            assert_int_equal(fx.submit_total, total_before);
        }

        /* P10: unchanged plan is deduplicated; heartbeat only re-sends the target. */
        fx.now_ns += 1000000ULL;
        iems_bess_result_t tick_result;
        int tick_rc = iems_bess_tick(bess, &tick_result);
        uint64_t after_short_tick = fx.submit_total;
        if (tick_rc >= 0) {
            if (r2.status >= 0 && !r2.low_soc_override) {
                assert_int_equal(after_short_tick, total_before);
            }
            fx.now_ns += 2000000000ULL;
            assert_int_equal(iems_bess_tick(bess, &tick_result), tick_rc);
            for (uint8_t i = 0; i < n; i++) {
                if (fabs(tick_result.cabinet[i].target_kw) > TEST_CLOSE) {
                    assert_true(fx.submit_calls[i] > 0u);
                    assert_int_equal(fx.last_issue_ts[i], fx.now_ns);
                }
            }
        }

        /* P7: a new fault in one cabinet must not change the others' states. */
        uint8_t k = (uint8_t)rnd_u32(n);
        iems_bess_result_t baseline = r2;
        fx.pcs[k].fault_bits |= 1u;
        int rc3 = iems_bess_request_power(bess, requested, 3, &r1);
        if (rc3 >= 0 && baseline.status >= 0) {
            for (uint8_t i = 0; i < n; i++) {
                if (i == k) continue;
                assert_int_equal(r1.cabinet[i].state, baseline.cabinet[i].state);
            }
            if (r1.cabinet[k].state == IEMS_BESS_CAB_FAULT ||
                r1.cabinet[k].state == IEMS_BESS_CAB_FAULT_LATCHED) {
                assert_close(r1.cabinet[k].target_kw, 0.0);
            }
        }

        /* P4: fixed snapshot, bigger request never shrinks a same-direction target. */
        iems_bess_storage_t storage2;
        iems_bess_t *bess2 = make_bess(&storage2, &cfg, &fx);
        assert_non_null(bess2);
        iems_bess_result_t small;
        iems_bess_result_t large;
        int rc_small = iems_bess_request_power(bess2, 50.0, 10, &small);
        int rc_large = iems_bess_request_power(bess2, 100.0, 11, &large);
        assert_int_equal(rc_small, rc_large);
        if (rc_small == 0) {
            for (uint8_t i = 0; i < n; i++) {
                if (small.cabinet[i].target_kw > TEST_CLOSE) {
                    assert_true(large.cabinet[i].target_kw >=
                                small.cabinet[i].target_kw - TEST_CLOSE);
                } else if (small.cabinet[i].target_kw < -TEST_CLOSE) {
                    assert_true(large.cabinet[i].target_kw <=
                                small.cabinet[i].target_kw + TEST_CLOSE);
                }
            }
        }
        iems_bess_destroy(bess2);
        iems_bess_destroy(bess);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_create_validation),
        cmocka_unit_test(test_value_updates),
        cmocka_unit_test(test_site_fault_v13),
        cmocka_unit_test(test_cabinet_faults_v6),
        cmocka_unit_test(test_availability_arbitration_v7),
        cmocka_unit_test(test_bms_port_failure_is_safe),
        cmocka_unit_test(test_onoff_connect),
        cmocka_unit_test(test_soc_hysteresis),
        cmocka_unit_test(test_low_soc_threshold_v8),
        cmocka_unit_test(test_v1_v2_v3),
        cmocka_unit_test(test_v4_v5),
        cmocka_unit_test(test_direction_blocked_sends_zero_v10),
        cmocka_unit_test(test_idempotency_v11),
        cmocka_unit_test(test_deadband_and_heartbeat_v14),
        cmocka_unit_test(test_clock_regression_is_safe),
        cmocka_unit_test(test_submit_failures_v12),
        cmocka_unit_test(test_stale_snapshot_and_old_sample_v7_p8),
        cmocka_unit_test(test_invalid_samples_are_finite),
        cmocka_unit_test(test_tick_epoch_and_cache),
        cmocka_unit_test(test_reactive_never_implicit_v15),
        cmocka_unit_test(test_properties_10k),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
