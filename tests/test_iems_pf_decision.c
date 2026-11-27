/**
 * @file test_iems_pf_decision.c
 * @brief Behavior tests for the pure PCC PF decision module (P2-03b slice 1a).
 *
 * Expected values are derived from the ticket five-column spec and iems_v3
 * strategy_pf_ctrl.c (pf_calculate_target, pf_check_deadband, pf_state_machine),
 * not from a run of the implementation:
 *   qt = |P| * tan(acos(min(|pf_target|, 1)))
 *   q_tol = min(0.5 * qt, 0.30)
 *   q_edge = qt - q_tol
 *   100 kW at 0.95 -> qt = 32.86841051788632, q_edge = 32.56841051788632
 *   100 kW at 0.97 -> qt = 25.062362435346852, q_edge = 24.762362435346852
 *   0.5 kW at 0.97 -> qt = 0.12531181217673426,
 *                     q_tol = 0.5*qt = 0.06265590608836713, q_edge = same
 *   100 kW at 1.0  -> qt = q_tol = q_edge = 0
 *
 * In-band hold (v3 strategy_pf_ctrl.c:669-673): after RUNNING, an in-band or
 * waiting tick emits the last dispatched Q, not 0, until RUNNING recomputes.
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "iems_pf_decision.h"

#define EPS 1e-9
#define QT_095 32.86841051788632
#define EDGE_095 32.56841051788632
#define QT_097 25.062362435346852
#define EDGE_097 24.762362435346852

static iems_pf_cfg_t base_config(void) {
    iems_pf_cfg_t cfg = {0};
    cfg.en_pf_ctrl = 1;
    cfg.reactive_mode = 1;
    cfg.pf_target = 0.95;
    cfg.pf_deadband = 0.02;
    cfg.p_min_kw = 2.5;
    cfg.start_delay_s = 0;
    cfg.hold_s = 3;
    cfg.control_cycle_s = 2;
    return cfg;
}

static iems_pf_input_t sample(uint64_t now_ms, double p_kw, double q_kvar, double pf,
                              bool meter_online) {
    iems_pf_input_t in = {0};
    in.now_ms = now_ms;
    in.p_kw = p_kw;
    in.q_kvar = q_kvar;
    in.pf = pf;
    in.meter_online = meter_online;
    return in;
}

/* Runs one evaluation and feeds next_state back, as the caller will. */
static iems_pf_decision_t eval_next(const iems_pf_cfg_t *cfg, iems_pf_state_t *state,
                                    const iems_pf_input_t *in) {
    iems_pf_decision_t decision = iems_pf_evaluate(cfg, state, in);
    *state = decision.next_state;
    return decision;
}

static void test_formula_golden(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t running = iems_pf_state_init();
    running.state = IEMS_PF_STATE_RUNNING;
    running.q_last = EDGE_095;
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    /* Out of band on the sigma side: the target is pulled to the band edge. */
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_int_equal(d.reason, IEMS_PF_REASON_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
    assert_float_equal(d.next_state.q_last, EDGE_095, EPS);

    /* Inside the band: no new dispatch; the last value is held (v3 :669-673). */
    in.q_kvar = 10.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_IN_BAND);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
    assert_float_equal(d.next_state.q_last, EDGE_095, EPS);

    /* Opposite-side deviation is pulled toward -q_edge, never across zero:
     * sign(q_target) == sign(q_pcc), not sign(sigma). */
    in.q_kvar = -40.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, -EDGE_095, EPS);

    /* Band boundary: |q_pcc| <= qt holds, one step beyond leaves the band. */
    in.q_kvar = 32.86;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_IN_BAND);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    in.q_kvar = 32.87;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    /* q_tol is pinned by the edge above: qt - 0.30, not qt - 0.5*qt (0.16). */

    /* Signed target: sigma = -1 mirrors the same two-sided behaviour. */
    cfg.pf_target = -0.95;
    in.q_kvar = -40.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, -EDGE_095, EPS);

    in.q_kvar = 40.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
}

static void test_low_power(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t running = iems_pf_state_init();
    running.state = IEMS_PF_STATE_RUNNING;
    iems_pf_input_t in = sample(0, 2.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    /* P = 2.0 kW < p_min 2.5: PF is meaningless, STANDBY with q = 0. */
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_LOW_POWER);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    assert_int_equal(d.next_state.timer, 0);

    in.p_kw = 2.499;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_LOW_POWER);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* Exactly p_min is not low; q = 0 is inside the band. */
    in.p_kw = 2.5;
    in.q_kvar = 0.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_IN_BAND);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* Above p_min and out of band -> RUNNING. */
    in.q_kvar = 10.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    double expected_edge_2p5 = 2.5 * tan(acos(0.95)) - 0.30;
    assert_true(fabs(d.q_target_kvar - expected_edge_2p5) < EPS);

    /* LOW_POWER clears the hold: it emits 0, unlike the in-band hold. */
    iems_pf_state_t held = iems_pf_state_init();
    held.state = IEMS_PF_STATE_RUNNING;
    held.q_last = EDGE_095;
    in = sample(0, 2.0, 40.0, 0.9, true);
    d = iems_pf_evaluate(&cfg, &held, &in);
    assert_int_equal(d.reason, IEMS_PF_REASON_LOW_POWER);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    assert_float_equal(d.next_state.q_last, 0.0, EPS);
}

static void test_start_delay_sequence(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    cfg.start_delay_s = 2;
    cfg.control_cycle_s = 1;
    iems_pf_state_t st = iems_pf_state_init();
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    /* First enabled tick only leaves DISABLED. */
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_ENABLED);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* Deviation starts at t=1000; one second later it is still STANDBY. */
    in.now_ms = 1000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    assert_true(d.next_state.timer != 0);

    in.now_ms = 2000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* At t=3000 the deviation has persisted 2 s -> RUNNING. */
    in.now_ms = 3000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_int_equal(d.reason, IEMS_PF_REASON_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
    assert_int_equal(d.next_state.timer, 0);

    in.now_ms = 4000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    /* Back inside the band -> STANDBY and the last dispatched q is held
     * (v3 strategy_pf_ctrl.c:669-673: PCS keeps its last command). */
    in.now_ms = 5000;
    in.q_kvar = 10.0;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_IN_BAND);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
    assert_float_equal(d.next_state.q_last, EDGE_095, EPS);
    assert_int_equal(d.next_state.timer, 0);

    /* The delay restarts from scratch after an in-band tick; waiting also
     * holds the last value. */
    in.now_ms = 6000;
    in.q_kvar = 40.0;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    in.now_ms = 7000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    in.now_ms = 8000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);
}

static void test_start_delay_boundaries(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t st;
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    /* start_delay 0 is valid in v3: the first out-of-band tick runs. */
    st = iems_pf_state_init();
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    in.now_ms = 1000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);

    /* Out-of-range falls back to 2 s: t=1000 starts, t=3000 switches. */
    cfg.start_delay_s = 61;
    st = iems_pf_state_init();
    in.now_ms = 0;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    in.now_ms = 1000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    in.now_ms = 2000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    in.now_ms = 3000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);

    /* Negative also falls back to 2 s. */
    cfg.start_delay_s = -1;
    st = iems_pf_state_init();
    in.now_ms = 0;
    eval_next(&cfg, &st, &in);
    in.now_ms = 1000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
}

static void test_enable_gate(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_RUNNING;
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    cfg.en_pf_ctrl = 0;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_DISABLED);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    assert_int_equal(d.next_state.timer, 0);

    cfg.en_pf_ctrl = 2; /* only exactly 1 enables */
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_DISABLED);

    cfg.en_pf_ctrl = 1;
    cfg.reactive_mode = 0;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_NO_OUTPUT);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* reactive_mode 2 participates; DISABLED -> STANDBY, q = 0. */
    cfg.reactive_mode = 2;
    st = iems_pf_state_init();
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_ENABLED);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* Enabling with an invalid meter goes straight to FAULT, not STANDBY. */
    st = iems_pf_state_init();
    in.meter_online = false;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_METER_OFFLINE);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
}

static void test_fault_and_recovery(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_RUNNING;
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    in.meter_online = false;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_METER_OFFLINE);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    in.meter_online = true;
    in.pf = 1.2;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_PF_OUT_OF_RANGE);

    in.pf = -1.2;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_PF_OUT_OF_RANGE);

    in.pf = NAN;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_BAD_MEASUREMENT);

    in.pf = 0.9;
    in.p_kw = NAN;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);
    assert_int_equal(d.reason, IEMS_PF_REASON_BAD_MEASUREMENT);

    /* Recovery: meter online and |pf| <= 1 -> STANDBY, one step. */
    st.state = IEMS_PF_STATE_FAULT;
    in = sample(2000, 100.0, 40.0, 0.9, true);
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_FAULT_RECOVERED);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    assert_int_equal(d.next_state.timer, 0);

    /* Still faulty stays FAULT. */
    st.state = IEMS_PF_STATE_FAULT;
    in.meter_online = false;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_FAULT);

    /* The enable gate outranks a fault. */
    cfg.en_pf_ctrl = 0;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_DISABLED);
}

static void test_config_fallbacks(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t running = iems_pf_state_init();
    running.state = IEMS_PF_STATE_RUNNING;
    iems_pf_input_t in = sample(0, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    /* |pf_target| outside [0.8, 1.0] falls back to 0.97, sign preserved. */
    cfg.pf_target = 1.5;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_097, EPS);

    cfg.pf_target = -1.5;
    in.q_kvar = -40.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, -EDGE_097, EPS);

    cfg.pf_target = 0.0;
    in.q_kvar = 40.0;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_097, EPS);

    cfg.pf_target = NAN;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_097, EPS);

    /* p_min_kw <= 0 or non-finite falls back to 2.5. */
    cfg = base_config();
    cfg.p_min_kw = 0.0;
    in = sample(0, 2.4, 40.0, 0.9, true);
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_LOW_POWER);

    cfg.p_min_kw = NAN;
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_LOW_POWER);
}

static void test_q_tol_small_p(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    cfg.p_min_kw = 0.1;
    cfg.pf_target = 0.97;
    iems_pf_state_t running = iems_pf_state_init();
    running.state = IEMS_PF_STATE_RUNNING;
    iems_pf_input_t in = sample(0, 0.5, 1.0, 0.9, true);
    iems_pf_decision_t d;

    /* P=0.5 kW, target 0.97: qt = 0.12531181217673426. The cap branch wins:
     * q_tol = min(0.5*qt, 0.30) = 0.06265590608836713 = q_edge. |Q|=1 is
     * out of band, so q_target is clipped to the edge. A fixed 0.30 tolerance
     * would make q_edge negative and emit -0.174688. */
    d = iems_pf_evaluate(&cfg, &running, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_int_equal(d.reason, IEMS_PF_REASON_RUNNING);
    assert_float_equal(d.q_target_kvar, 0.06265590608836713, EPS);
    assert_float_equal(d.next_state.q_last, 0.06265590608836713, EPS);
}

static void test_closed_interval_boundaries(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t st;
    iems_pf_input_t in;
    iems_pf_decision_t d;

    /* |pf_target| = 1.0 is inside the valid interval, not the 0.97 fallback:
     * qt = tan(acos(1)) * |P| = 0, q_tol = 0, q_edge = 0, q_target = 0. */
    cfg.pf_target = 1.0;
    st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_RUNNING;
    in = sample(0, 100.0, 40.0, 0.9, true);
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_int_equal(d.reason, IEMS_PF_REASON_RUNNING);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* start_delay_s = 60 is inside the valid interval: the deviation starts
     * at t=0 and only the 61st out-of-band tick (t=60000) is RUNNING. */
    cfg = base_config();
    cfg.start_delay_s = 60;
    st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_STANDBY;
    in = sample(0, 100.0, 40.0, 0.9, true);
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    for (uint64_t t = 1000; t < 60000; t += 1000) {
        in.now_ms = t;
        d = eval_next(&cfg, &st, &in);
        assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
        assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    }
    in.now_ms = 60000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);
    assert_int_equal(d.reason, IEMS_PF_REASON_RUNNING);
    assert_float_equal(d.q_target_kvar, EDGE_095, EPS);

    /* A deviation starting at now_ms = 0 keeps the +1 timer sentinel: t=1000
     * is still waiting, t=2000 has elapsed the 2 s delay. */
    cfg = base_config();
    cfg.start_delay_s = 2;
    st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_STANDBY;
    in = sample(0, 100.0, 40.0, 0.9, true);
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    in.now_ms = 1000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    in.now_ms = 2000;
    d = eval_next(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_RUNNING);

    /* At now_ms = UINT64_MAX the timer saturates instead of wrapping to 0,
     * which would read as "not timing" forever. */
    st = iems_pf_state_init();
    st.state = IEMS_PF_STATE_STANDBY;
    in = sample(UINT64_MAX, 100.0, 40.0, 0.9, true);
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.reason, IEMS_PF_REASON_WAITING_START);
    assert_true(d.next_state.timer != 0);
}

static void test_bad_config(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    iems_pf_state_t st = iems_pf_state_init();
    iems_pf_input_t in = sample(5000, 100.0, 40.0, 0.9, true);
    iems_pf_decision_t d;

    d = iems_pf_evaluate(NULL, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_BAD_CONFIG);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
    /* Contract: a NULL cfg still advances last_ts when the input is present. */
    assert_true(d.next_state.last_ts == in.now_ms);

    d = iems_pf_evaluate(&cfg, &st, NULL);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_BAD_CONFIG);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);

    /* A NULL state is a fresh DISABLED state, not a crash. */
    d = iems_pf_evaluate(&cfg, NULL, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_STANDBY);
    assert_int_equal(d.reason, IEMS_PF_REASON_ENABLED);

    /* A carried state outside the enum is rejected, not trusted. */
    st.state = (iems_pf_status_t)99;
    d = iems_pf_evaluate(&cfg, &st, &in);
    assert_int_equal(d.state, IEMS_PF_STATE_DISABLED);
    assert_int_equal(d.reason, IEMS_PF_REASON_BAD_CONFIG);
    assert_float_equal(d.q_target_kvar, 0.0, EPS);
}

static uint32_t g_rng;

static uint32_t rng_next(void) {
    g_rng = g_rng * 1664525u + 1013904223u;
    return g_rng;
}

static double random_between(double lo, double hi) {
    return lo + (hi - lo) * ((double)(rng_next() >> 8) / 16777216.0);
}

static bool transition_legal(iems_pf_status_t from, iems_pf_status_t to) {
    switch (from) {
        case IEMS_PF_STATE_DISABLED:
            return to == IEMS_PF_STATE_DISABLED || to == IEMS_PF_STATE_STANDBY ||
                   to == IEMS_PF_STATE_FAULT;
        case IEMS_PF_STATE_STANDBY:
            return to == IEMS_PF_STATE_STANDBY || to == IEMS_PF_STATE_RUNNING ||
                   to == IEMS_PF_STATE_FAULT || to == IEMS_PF_STATE_DISABLED;
        case IEMS_PF_STATE_RUNNING:
            return to == IEMS_PF_STATE_RUNNING || to == IEMS_PF_STATE_STANDBY ||
                   to == IEMS_PF_STATE_FAULT || to == IEMS_PF_STATE_DISABLED;
        case IEMS_PF_STATE_FAULT:
            return to == IEMS_PF_STATE_FAULT || to == IEMS_PF_STATE_STANDBY ||
                   to == IEMS_PF_STATE_DISABLED;
        default:
            return false;
    }
}

static void test_property_10k(void **state) {
    (void)state;
    iems_pf_cfg_t cfg = base_config();
    cfg.start_delay_s = 1;
    cfg.control_cycle_s = 2;
    iems_pf_state_t st = iems_pf_state_init();
    g_rng = 0x5eed03bcu;

    uint64_t now = 0;
    int violations = 0;
    int transitions = 0;
    int waiting_ticks = 0;
    bool states_seen[4] = {false, false, false, false};
    const double qt_095 = QT_095 / 100.0;

    for (int i = 0; i < 10000; i++) {
        uint32_t r = rng_next();

        if (i > 0 && (r % 97u) == 0u) {
            now = now > 500u ? now - 500u : 0u; /* occasional backwards clock */
        } else {
            now += r % 1500u;
        }

        cfg.en_pf_ctrl = (r % 53u) == 0u ? 0 : 1;
        cfg.reactive_mode = (r % 61u) == 0u ? 0 : 1;

        /* Occasionally inject an arbitrary carried state so the sequence is
         * not limited to states this implementation produced, while staying
         * within the four-state contract. */
        if (i > 0 && (r % 499u) == 0u) {
            st.state = (iems_pf_status_t)(rng_next() % 4u);
            st.timer = ((rng_next() & 1u) != 0u) ? now + 1u : 0u;
            st.last_ts = now;
        }

        iems_pf_input_t in = sample(now, random_between(-150.0, 150.0), random_between(-80.0, 80.0),
                                    random_between(-1.2, 1.2), (r % 11u) != 0u);

        iems_pf_state_t prev = st;
        iems_pf_decision_t d = iems_pf_evaluate(&cfg, &prev, &in);
        iems_pf_decision_t again = iems_pf_evaluate(&cfg, &prev, &in);

        /* Same cfg/state/input must give the same decision. */
        if (d.q_target_kvar != again.q_target_kvar || d.state != again.state ||
            d.reason != again.reason || d.next_state.state != again.next_state.state ||
            d.next_state.timer != again.next_state.timer ||
            d.next_state.q_last != again.next_state.q_last ||
            d.next_state.last_ts != again.next_state.last_ts) {
            violations++;
        }

        if (d.state != d.next_state.state) violations++;
        if ((unsigned)d.state > 3u) violations++;
        if (!transition_legal(prev.state, d.next_state.state)) violations++;
        if (d.next_state.last_ts != in.now_ms) violations++;
        if (d.next_state.q_last != d.q_target_kvar) violations++;

        if (d.next_state.timer != 0u && d.reason != IEMS_PF_REASON_WAITING_START) {
            violations++;
        }
        if (d.reason == IEMS_PF_REASON_WAITING_START && d.state != IEMS_PF_STATE_STANDBY) {
            violations++;
        }

        if (d.state == IEMS_PF_STATE_RUNNING) {
            double qt = fabs(in.p_kw) * qt_095;
            double q_tol = fmin(0.5 * qt, 0.30);
            double edge = qt - q_tol;
            if (!isfinite(d.q_target_kvar)) violations++;
            if (fabs(d.q_target_kvar) > qt + EPS) violations++;
            if (fabs(in.q_kvar) <= qt) violations++; /* RUNNING is out of band */
            if (in.q_kvar != 0.0 && d.q_target_kvar != 0.0 &&
                ((d.q_target_kvar > 0.0) != (in.q_kvar > 0.0))) {
                violations++;
            }
            if (fabs(in.q_kvar) > edge && fabs(fabs(d.q_target_kvar) - edge) > EPS) {
                violations++;
            }
        } else if (d.state == IEMS_PF_STATE_STANDBY &&
                   (d.reason == IEMS_PF_REASON_ENABLED || d.reason == IEMS_PF_REASON_IN_BAND ||
                    d.reason == IEMS_PF_REASON_WAITING_START)) {
            /* Hold states emit the last dispatched Q (0 on the first tick). */
            if (d.q_target_kvar != prev.q_last) violations++;
        } else if (d.q_target_kvar != 0.0) {
            violations++;
        }

        if (cfg.en_pf_ctrl != 1 || cfg.reactive_mode == 0) {
            if (d.state != IEMS_PF_STATE_DISABLED) violations++;
        } else if (!in.meter_online || fabs(in.pf) > 1.0) {
            if (d.state != IEMS_PF_STATE_FAULT) violations++;
        } else if (fabs(in.p_kw) < 2.5) {
            if (d.state != IEMS_PF_STATE_STANDBY) violations++;
        }

        /* RUNNING may only be entered after the delay elapsed (or delay 0). */
        if (d.state == IEMS_PF_STATE_RUNNING && prev.state == IEMS_PF_STATE_STANDBY) {
            if (prev.timer == 0u) {
                if (cfg.start_delay_s != 0) violations++;
            } else {
                uint64_t start = prev.timer - 1u;
                uint64_t elapsed = in.now_ms > start ? in.now_ms - start : 0u;
                if (elapsed < (uint64_t)cfg.start_delay_s * 1000u) violations++;
            }
        }

        if (d.reason == IEMS_PF_REASON_WAITING_START) waiting_ticks++;
        if (prev.state != d.next_state.state) transitions++;
        states_seen[d.state] = true;
        st = d.next_state;
    }

    printf(
        "property: 10000 ticks, %d transitions, %d waiting-start ticks, "
        "states seen disabled=%d standby=%d running=%d fault=%d, %d violations\n",
        transitions, waiting_ticks, states_seen[IEMS_PF_STATE_DISABLED],
        states_seen[IEMS_PF_STATE_STANDBY], states_seen[IEMS_PF_STATE_RUNNING],
        states_seen[IEMS_PF_STATE_FAULT], violations);
    assert_int_equal(violations, 0);
    assert_true(transitions > 0);
    assert_true(waiting_ticks > 0);
    assert_true(states_seen[IEMS_PF_STATE_DISABLED]);
    assert_true(states_seen[IEMS_PF_STATE_STANDBY]);
    assert_true(states_seen[IEMS_PF_STATE_RUNNING]);
    assert_true(states_seen[IEMS_PF_STATE_FAULT]);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_formula_golden),       cmocka_unit_test(test_low_power),
        cmocka_unit_test(test_start_delay_sequence), cmocka_unit_test(test_start_delay_boundaries),
        cmocka_unit_test(test_enable_gate),          cmocka_unit_test(test_fault_and_recovery),
        cmocka_unit_test(test_config_fallbacks),     cmocka_unit_test(test_bad_config),
        cmocka_unit_test(test_q_tol_small_p),        cmocka_unit_test(test_closed_interval_boundaries),
        cmocka_unit_test(test_property_10k),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
