/**
 * @file test_iems_soc_keep.c
 * @brief Behavior tests for the pure backup SOC-keep strategy module.
 *
 * Expected values come from iems_v3 Strategy_SocKeep_cfg_update /
 * Strategy_SocKeep_status_check / Strategy_SocKeep_Handler and
 * delay_trigger_bool_check, not from a run of the implementation.
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "iems_soc_keep.h"

#define EPS 1e-9

static iems_soc_keep_cfg_t base_config(void) {
    iems_soc_keep_cfg_t cfg = {0};
    cfg.enable = true;
    cfg.active_soc_pct = 50.0;
    cfg.inactive_soc_pct = 55.0;
    cfg.charge_power_kw = 8.0; /* > 5 -> -fabs(8) = -8 kW */
    cfg.max_charge_power_kw = 20.0;
    cfg.delay_on_ms = 3000;
    cfg.delay_off_ms = 3000;
    return cfg;
}

static iems_soc_keep_input_t tick(uint64_t now_ms, double soc_pct) {
    iems_soc_keep_input_t in = {0};
    in.now_ms = now_ms;
    in.soc_pct = soc_pct;
    in.strategy_type = IEMS_SOC_KEEP_STRATEGY_PERIOD_CTRL;
    in.enabled = true;
    return in;
}

/* Runs one evaluation and feeds next_state back, as the caller will. */
static iems_soc_keep_decision_t eval_at(const iems_soc_keep_cfg_t* cfg,
                                        iems_soc_keep_state_t* state, uint64_t now_ms,
                                        double soc_pct) {
    iems_soc_keep_input_t in = tick(now_ms, soc_pct);
    iems_soc_keep_decision_t decision = iems_soc_keep_evaluate(cfg, state, &in);
    *state = decision.next_state;
    return decision;
}

static void test_hysteresis(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    /* SOC 60 -> 49: enter after 3 s. v3: SOC < active_soc (50). */
    d = eval_at(&cfg, &st, 0, 60.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_int_equal(d.next_state.pending_target_status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    d = eval_at(&cfg, &st, 0, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_int_equal(d.next_state.pending_target_status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_SOC_BELOW_ACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    d = eval_at(&cfg, &st, 1000, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_int_equal(d.next_state.pending_target_status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    d = eval_at(&cfg, &st, 3000, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_int_equal(d.next_state.pending_target_status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    /* SOC 56 >= inactive 55: exit after 3 s; v3 keeps charging meanwhile. */
    d = eval_at(&cfg, &st, 5600, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    assert_int_equal(d.next_state.pending_target_status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    d = eval_at(&cfg, &st, 8600, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* 49 -> 51 -> 49 within 3 s never reaches ACTIVE; the timer restarts. */
    st = iems_soc_keep_state_init();
    eval_at(&cfg, &st, 0, 49.0);
    d = eval_at(&cfg, &st, 1000, 51.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    d = eval_at(&cfg, &st, 2000, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    d = eval_at(&cfg, &st, 3000, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_delay_boundaries(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    /* Enter delay: 2999 ms does not switch, 3000 ms does. */
    eval_at(&cfg, &st, 0, 49.0);
    d = eval_at(&cfg, &st, 2999, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    d = eval_at(&cfg, &st, 3000, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_ACTIVE);

    /* Exit delay: 2999 ms still charges, 3000 ms switches and zeroes. */
    d = eval_at(&cfg, &st, 4000, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    assert_float_equal(d.target_power_kw, -8.0, EPS);
    d = eval_at(&cfg, &st, 6999, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);
    d = eval_at(&cfg, &st, 7000, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* Enter condition disappears mid-delay: back to INACTIVE, timer cleared. */
    st = iems_soc_keep_state_init();
    eval_at(&cfg, &st, 0, 49.0);
    d = eval_at(&cfg, &st, 1500, 50.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_int_equal(d.next_state.pending_since_ms, 0);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* Exit condition disappears mid-delay: back to ACTIVE and charging. */
    st = iems_soc_keep_state_init();
    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0); /* ACTIVE */
    d = eval_at(&cfg, &st, 4000, 56.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    d = eval_at(&cfg, &st, 4500, 54.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);
}

static void test_threshold_resolution(void** state) {
    (void)state;
    double active = -1.0;
    double inactive = -1.0;

    /* v3 Strategy_SocKeep_cfg_update: inactive = active + margin. */
    assert_true(iems_soc_keep_resolve_thresholds(50.0, 5.0, &active, &inactive));
    assert_float_equal(active, 50.0, EPS);
    assert_float_equal(inactive, 55.0, EPS);

    /* margin <= 0 falls back to +5. */
    assert_true(iems_soc_keep_resolve_thresholds(50.0, 0.0, &active, &inactive));
    assert_float_equal(inactive, 55.0, EPS);
    assert_true(iems_soc_keep_resolve_thresholds(50.0, -3.0, &active, &inactive));
    assert_float_equal(inactive, 55.0, EPS);

    /* Both thresholds clamp to 0..100. */
    assert_true(iems_soc_keep_resolve_thresholds(98.0, 5.0, &active, &inactive));
    assert_float_equal(active, 98.0, EPS);
    assert_float_equal(inactive, 100.0, EPS);
    assert_true(iems_soc_keep_resolve_thresholds(100.0, 5.0, &active, &inactive));
    assert_float_equal(active, 100.0, EPS);
    assert_float_equal(inactive, 100.0, EPS);

    /* active itself clamps too: 150 -> 100, and 100 + 5 -> 100. */
    assert_true(iems_soc_keep_resolve_thresholds(150.0, 5.0, &active, &inactive));
    assert_float_equal(active, 100.0, EPS);
    assert_float_equal(inactive, 100.0, EPS);

    /* NaN, infinite or negative active is BAD_CONFIG, never guessed. */
    assert_false(iems_soc_keep_resolve_thresholds(NAN, 5.0, &active, &inactive));
    assert_false(iems_soc_keep_resolve_thresholds(INFINITY, 5.0, &active, &inactive));
    assert_false(iems_soc_keep_resolve_thresholds(-1.0, 5.0, &active, &inactive));
    assert_false(iems_soc_keep_resolve_thresholds(50.0, NAN, &active, &inactive));
    assert_false(iems_soc_keep_resolve_thresholds(50.0, 5.0, NULL, &inactive));
    assert_false(iems_soc_keep_resolve_thresholds(50.0, 5.0, &active, NULL));
}

static void test_charge_power_selection(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0); /* ACTIVE */

    /* charge_power_kw 8 > 5 -> -8. */
    d = eval_at(&cfg, &st, 4000, 49.0);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    /* Exactly 5 is not > 5 -> -max_charge_power_kw. */
    cfg.charge_power_kw = 5.0;
    d = eval_at(&cfg, &st, 4100, 49.0);
    assert_float_equal(d.target_power_kw, -20.0, EPS);

    /* fabs() applies, but only above the 5 kW gate. */
    cfg.charge_power_kw = 5.1;
    d = eval_at(&cfg, &st, 4200, 49.0);
    assert_float_equal(d.target_power_kw, -5.1, EPS);
    cfg.charge_power_kw = -8.0;
    d = eval_at(&cfg, &st, 4300, 49.0);
    assert_float_equal(d.target_power_kw, -20.0, EPS);

    /* max_charge_power_kw 0 still yields a non-positive target. */
    cfg.charge_power_kw = 0.0;
    cfg.max_charge_power_kw = 0.0;
    d = eval_at(&cfg, &st, 4400, 49.0);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_bad_config(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_input_t in = tick(0, 40.0);
    iems_soc_keep_decision_t d;

    /* Establish ACTIVE first; a bad config resets to INACTIVE, 0 power. */
    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0);
    cfg.active_soc_pct = NAN;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    cfg = base_config();
    cfg.active_soc_pct = -1.0;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    cfg = base_config();
    cfg.inactive_soc_pct = 49.0; /* active 50 > inactive 49 */
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    cfg = base_config();
    cfg.inactive_soc_pct = NAN;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    cfg = base_config();
    cfg.charge_power_kw = NAN;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    cfg = base_config();
    cfg.max_charge_power_kw = -1.0;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    cfg = base_config();
    iems_soc_keep_input_t bad_soc = in;
    bad_soc.soc_pct = NAN;
    d = iems_soc_keep_evaluate(&cfg, &st, &bad_soc);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);

    d = iems_soc_keep_evaluate(NULL, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);
    d = iems_soc_keep_evaluate(&cfg, NULL, NULL);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_zero_delay_rejected(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_input_t in = tick(0, 49.0);
    iems_soc_keep_decision_t d;

    /* Invariant: ACTIVE is only reachable through PENDING_ON. A zero enter
     * delay would skip it, so the config is BAD_CONFIG, not a shortcut. */
    cfg.delay_on_ms = 0;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* Same for the exit delay: PENDING_OFF must not be skippable. */
    cfg = base_config();
    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0); /* ACTIVE */
    cfg.delay_off_ms = 0;
    d = eval_at(&cfg, &st, 4000, 56.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_BAD_CONFIG);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_disabled_and_unsupported(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    /* ACTIVE, then cfg.enable=false: immediate INACTIVE, no exit delay. */
    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0);
    cfg.enable = false;
    d = eval_at(&cfg, &st, 3100, 49.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_DISABLED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_int_equal(d.next_state.pending_since_ms, 0);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* Runtime enable false behaves the same. */
    cfg.enable = true;
    iems_soc_keep_input_t in = tick(3200, 49.0);
    in.enabled = false;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_DISABLED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);

    /* v3 supports only PERIOD_CTRL and PV_STORAGE. */
    in = tick(3300, 49.0);
    in.strategy_type = 1; /* v3 DEMAND_FIXED */
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_UNSUPPORTED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);

    in.strategy_type = IEMS_SOC_KEEP_STRATEGY_PV_STORAGE;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_disabled_unsupported_before_stale(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    /* ACTIVE at t=3000; the ticks below go backwards to t=1000. */
    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0);
    assert_int_equal(st.status, IEMS_SOC_KEEP_STATUS_ACTIVE);

    /* A4 outranks A5: a disabled strategy is not evaluating, so the stale
     * clock cannot mask the reset to INACTIVE with 0 power. */
    cfg.enable = false;
    d = eval_at(&cfg, &st, 1000, 49.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_DISABLED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_int_equal(d.next_state.pending_since_ms, 0);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* The runtime enable flag behaves the same on a backwards tick. */
    cfg.enable = true;
    iems_soc_keep_input_t in = tick(1000, 49.0);
    in.enabled = false;
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_DISABLED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);

    /* Unsupported strategy likewise outranks STALE. */
    in = tick(1000, 49.0);
    in.strategy_type = 1; /* v3 DEMAND_FIXED */
    d = iems_soc_keep_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_UNSUPPORTED);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_stale_clock(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0); /* ACTIVE, last_now 3000 */

    d = eval_at(&cfg, &st, 2999, 49.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_STALE);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_ACTIVE);
    assert_int_equal(d.next_state.last_now_ms, 3000);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    /* A later, monotonic tick resumes normally. */
    d = eval_at(&cfg, &st, 5000, 49.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_ACTIVE);

    /* PENDING_ON also holds its state and pending_since on regression. */
    st = iems_soc_keep_state_init();
    eval_at(&cfg, &st, 0, 49.0);
    d = eval_at(&cfg, &st, 10, 49.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    d = eval_at(&cfg, &st, 5, 49.0);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_STALE);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_ON);
    assert_int_equal(d.next_state.pending_since_ms, 0);
    assert_int_equal(d.next_state.last_now_ms, 10);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static void test_exit_threshold_equality(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    iems_soc_keep_decision_t d;

    eval_at(&cfg, &st, 0, 49.0);
    eval_at(&cfg, &st, 3000, 49.0); /* ACTIVE */
    assert_int_equal(st.status, IEMS_SOC_KEEP_STATUS_ACTIVE);

    /* v3 exits at SOC >= inactive_soc_pct; equality (55) is an exit, not a
     * hold. The delay then charges on the way out and zeroes at expiry. */
    d = eval_at(&cfg, &st, 4000, 55.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    assert_int_equal(d.reason, IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE);
    assert_true(d.active);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    d = eval_at(&cfg, &st, 6999, 55.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_PENDING_OFF);
    assert_float_equal(d.target_power_kw, -8.0, EPS);

    d = eval_at(&cfg, &st, 7000, 55.0);
    assert_int_equal(d.next_state.status, IEMS_SOC_KEEP_STATUS_INACTIVE);
    assert_false(d.active);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

static uint32_t g_rng;

static uint32_t rng_next(void) {
    g_rng = g_rng * 1664525u + 1013904223u;
    return g_rng;
}

static void test_property_10k(void** state) {
    (void)state;
    iems_soc_keep_cfg_t cfg = base_config();
    iems_soc_keep_state_t st = iems_soc_keep_state_init();
    g_rng = 0x5eed1234u;

    double soc = 50.0;
    uint64_t now = 0;
    int violations = 0;
    int transitions = 0;
    int stale_ticks = 0;
    int active_ticks = 0;
    int pending_off_ticks = 0;

    for (int i = 0; i < 10000; i++) {
        uint32_t r = rng_next();
        soc += ((double)(r % 1201u) - 600.0) / 100.0; /* -6.00 .. +6.00 */
        if (soc < 0.0) soc = 0.0;
        if (soc > 100.0) soc = 100.0;

        bool regress = i > 0 && (r % 997u) == 0u;
        iems_soc_keep_input_t in = tick(now, soc);
        if (regress) {
            uint64_t back = 1u + (r % 500u);
            in.now_ms = now > back ? now - back : 0u;
        } else {
            now += 200u + (r % 401u); /* 200..600 ms */
            in.now_ms = now;
        }

        iems_soc_keep_status_t prev_status = st.status;
        iems_soc_keep_status_t prev_target = st.pending_target_status;
        uint64_t prev_since = st.pending_since_ms;
        uint64_t prev_last = st.last_now_ms;
        bool prev_first = st.first;

        iems_soc_keep_decision_t d = iems_soc_keep_evaluate(&cfg, &st, &in);

        /* pending_target_status is derived and must never disagree. */
        iems_soc_keep_status_t expected_target =
            d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_ON ? IEMS_SOC_KEEP_STATUS_ACTIVE
            : d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_OFF
                ? IEMS_SOC_KEEP_STATUS_INACTIVE
                : d.next_state.status;
        if (d.next_state.pending_target_status != expected_target) violations++;

        /* Invariants: power <= 0; ACTIVE/PENDING_OFF charge, others zero. */
        if (d.target_power_kw > 0.0) violations++;
        switch (d.next_state.status) {
            case IEMS_SOC_KEEP_STATUS_ACTIVE:
            case IEMS_SOC_KEEP_STATUS_PENDING_OFF:
                if (!d.active) violations++;
                if (fabs(d.target_power_kw + 8.0) > EPS) violations++;
                break;
            case IEMS_SOC_KEEP_STATUS_INACTIVE:
            case IEMS_SOC_KEEP_STATUS_PENDING_ON:
                if (d.active) violations++;
                if (d.target_power_kw != 0.0) violations++;
                break;
            default:
                violations++;
                break;
        }
        if (d.next_state.status == IEMS_SOC_KEEP_STATUS_ACTIVE) active_ticks++;
        if (d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_OFF) pending_off_ticks++;
        if (d.next_state.status != prev_status) transitions++;

        if (regress) {
            stale_ticks++;
            if (d.reason != IEMS_SOC_KEEP_REASON_STALE) violations++;
            if (d.next_state.status != prev_status ||
                d.next_state.pending_target_status != prev_target ||
                d.next_state.pending_since_ms != prev_since ||
                d.next_state.last_now_ms != prev_last || d.next_state.first != prev_first) {
                violations++;
            }
        } else {
            /* At most one transition per tick; delays are 3000 ms and a tick
             * advances at most 600 ms, so no pending state may be skipped. */
            bool legal = false;
            switch (prev_status) {
                case IEMS_SOC_KEEP_STATUS_INACTIVE:
                    legal = d.next_state.status == IEMS_SOC_KEEP_STATUS_INACTIVE ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_ON;
                    break;
                case IEMS_SOC_KEEP_STATUS_PENDING_ON:
                    legal = d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_ON ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_INACTIVE ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_ACTIVE;
                    if (d.next_state.status == IEMS_SOC_KEEP_STATUS_ACTIVE &&
                        in.now_ms - prev_since < cfg.delay_on_ms) {
                        violations++;
                    }
                    break;
                case IEMS_SOC_KEEP_STATUS_ACTIVE:
                    legal = d.next_state.status == IEMS_SOC_KEEP_STATUS_ACTIVE ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_OFF;
                    break;
                case IEMS_SOC_KEEP_STATUS_PENDING_OFF:
                    legal = d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_OFF ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_ACTIVE ||
                            d.next_state.status == IEMS_SOC_KEEP_STATUS_INACTIVE;
                    if (d.next_state.status == IEMS_SOC_KEEP_STATUS_INACTIVE &&
                        in.now_ms - prev_since < cfg.delay_off_ms) {
                        violations++;
                    }
                    break;
                default:
                    break;
            }
            if (!legal) violations++;
            /* A new pending window starts at this tick; an ongoing one keeps
             * its original start. */
            if (d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_ON &&
                prev_status == IEMS_SOC_KEEP_STATUS_INACTIVE &&
                d.next_state.pending_since_ms != in.now_ms) {
                violations++;
            }
            if (d.next_state.status == IEMS_SOC_KEEP_STATUS_PENDING_OFF &&
                prev_status == IEMS_SOC_KEEP_STATUS_ACTIVE &&
                d.next_state.pending_since_ms != in.now_ms) {
                violations++;
            }
        }

        st = d.next_state;
    }

    printf(
        "property: 10000 ticks, %d transitions, %d stale ticks, %d active ticks, %d pending-off "
        "ticks, %d violations\n",
        transitions, stale_ticks, active_ticks, pending_off_ticks, violations);
    assert_int_equal(violations, 0);
    /* The sequence must actually exercise both sides of the state machine. */
    assert_true(transitions > 0);
    assert_true(stale_ticks > 0);
    assert_true(active_ticks > 0);
    assert_true(pending_off_ticks > 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hysteresis),
        cmocka_unit_test(test_delay_boundaries),
        cmocka_unit_test(test_threshold_resolution),
        cmocka_unit_test(test_charge_power_selection),
        cmocka_unit_test(test_bad_config),
        cmocka_unit_test(test_zero_delay_rejected),
        cmocka_unit_test(test_disabled_and_unsupported),
        cmocka_unit_test(test_disabled_unsupported_before_stale),
        cmocka_unit_test(test_stale_clock),
        cmocka_unit_test(test_exit_threshold_equality),
        cmocka_unit_test(test_property_10k),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
