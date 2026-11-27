/**
 * @file iems_pf_decision.h
 * @brief P2-03b: pure PCC power-factor decision module.
 *
 * Data shape: the caller injects the v3 PF settings and one PCC meter sample
 * per control cycle; the evaluator returns the station-level Q target plus the
 * state to feed back next cycle. It performs no IO, reads no clock and keeps
 * no mutable globals, so the same cfg/state/input always produce the same
 * decision.
 *
 * States (v3 pf_ctrl_state_t order; published as app.ems.strategy_state 0..3):
 *
 *   from      condition                               to        q
 *   DISABLED  en_pf_ctrl != 1                         DISABLED  0
 *   DISABLED  reactive_mode == 0                      DISABLED  0
 *   DISABLED  enabled, meter/pf invalid               FAULT     0
 *   DISABLED  enabled, meter valid                    STANDBY   q_last (first 0)
 *   STANDBY   enable gate fails                       DISABLED  0
 *   STANDBY   meter offline / |pf| > 1 / non-finite   FAULT     0
 *   STANDBY   |P| < p_min_kw                          STANDBY   0
 *   STANDBY   |q_pcc * sigma| <= qt                   STANDBY   q_last
 *   STANDBY   out of band, elapsed < start_delay_s    STANDBY   q_last
 *   STANDBY   out of band, elapsed >= start_delay_s   RUNNING   q_target
 *   RUNNING   enable gate fails                       DISABLED  0
 *   RUNNING   meter offline / |pf| > 1 / non-finite   FAULT     0
 *   RUNNING   |P| < p_min_kw                          STANDBY   0
 *   RUNNING   in band                                 STANDBY   q_last
 *   RUNNING   out of band                             RUNNING   q_target
 *   FAULT     enable gate fails                       DISABLED  0
 *   FAULT     meter/pf still bad                      FAULT     0
 *   FAULT     meter online, |pf| <= 1                 STANDBY   0
 *
 * One transition at most per evaluation. FAULT recovery here only checks the
 * meter; v3 also requires an available PCS, which is outside this module.
 *
 * q_last is the last emitted q_target. The ENABLED/IN_BAND/WAITING_START rows
 * repeat it, so an in-band or waiting tick keeps the PCS on its last command
 * (v3 strategy_pf_ctrl.c:669-673); every other non-RUNNING row emits 0 and
 * clears q_last with it. The returned q_target is the actual command, not a
 * delta, so the caller publishes it on every evaluation.
 *
 * Q target (v3 strategy_pf_ctrl.c:163-193, meter coordinates):
 *
 *   sigma    = pf_target >= 0 ? +1 : -1
 *   qt       = |P| * tan(acos(min(|pf_target|, 1)))
 *   q_tol    = min(0.5 * qt, 0.30)
 *   q_edge   = qt - q_tol
 *   q_ref    = clip(q_pcc * sigma, -q_edge, +q_edge)
 *   q_target = q_ref * sigma
 *
 * The band is symmetric in the sigma-aligned frame: in band means
 * |q_pcc * sigma| <= qt. A RUNNING q_target is |q_target| <= q_edge <= qt and
 * keeps the measured Q side while pulling it toward the band:
 * sign(q_target) == sign(q_pcc) whenever q_target != 0. It equals sigma only
 * when q_pcc is already on the sigma side. An opposite-side deviation is
 * pulled to -q_edge * sigma, never across zero. A held q_last is exempt from
 * the current band: it was computed against the P at dispatch time.
 *
 * v3 fallbacks applied to the raw settings: |pf_target| outside [0.8, 1.0]
 * becomes +/-0.97 (sign preserved; NaN and -Inf become +0.97, where v3 keeps a
 * defective value), p_min_kw <= 0 or non-finite becomes 2.5 (v3 keeps +Inf;
 * falling back to 2.5 is safer), start_delay_s outside [0, 60] becomes 2.
 * Non-finite P/Q/PF forces FAULT, hardening beyond the stated spec.
 * pf_deadband (0.02), hold_s (3) and control_cycle_s (2) are carried for
 * parity and later slices; this decision does not use them. control_cycle_s
 * defaults to 2 s because v3's runtime config initializer is 2 (jsonct.c:59);
 * its loader falls back to 1 s only for an out-of-range value
 * (strategy_pf_ctrl.c:106-109), and P5A prefers 1 s. The ticket keeps
 * p_min_kw at 2.5; v3's jsonct.c:61 runtime value is 5 (recorded residual).
 *
 * Not in this slice: hold_s timing, slope/ramp, PID, gain identification,
 * overshoot/no-response detection and per-PCS allocation. v3's PCS-side hold
 * timer and FAULT ramp-down belong to the consumer or a later slice. The
 * caller evaluates once per control_cycle_s and publishes the returned
 * q_target every time.
 */

#ifndef IEMS_PF_DECISION_H
#define IEMS_PF_DECISION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* v3 defaults/fallbacks (jsonct.c:57-62, strategy_pf_ctrl.c:85-120). */
#define IEMS_PF_DEFAULT_PF_TARGET 0.97
#define IEMS_PF_DEFAULT_DEADBAND 0.02
#define IEMS_PF_DEFAULT_P_MIN_KW 2.5
#define IEMS_PF_DEFAULT_START_DELAY_S 2
#define IEMS_PF_DEFAULT_HOLD_S 3
#define IEMS_PF_DEFAULT_CONTROL_CYCLE_S 2

typedef enum {
    IEMS_PF_STATE_DISABLED = 0,
    IEMS_PF_STATE_STANDBY = 1,
    IEMS_PF_STATE_RUNNING = 2,
    IEMS_PF_STATE_FAULT = 3
} iems_pf_status_t;

typedef enum {
    IEMS_PF_REASON_NONE = 0,
    IEMS_PF_REASON_BAD_CONFIG,
    IEMS_PF_REASON_DISABLED,  /* en_pf_ctrl != 1 */
    IEMS_PF_REASON_NO_OUTPUT, /* reactive_mode == 0 */
    IEMS_PF_REASON_METER_OFFLINE,
    IEMS_PF_REASON_PF_OUT_OF_RANGE,
    IEMS_PF_REASON_BAD_MEASUREMENT, /* non-finite P/Q/PF */
    IEMS_PF_REASON_LOW_POWER,
    IEMS_PF_REASON_IN_BAND,
    IEMS_PF_REASON_WAITING_START,
    IEMS_PF_REASON_RUNNING,
    IEMS_PF_REASON_ENABLED, /* DISABLED -> STANDBY */
    IEMS_PF_REASON_FAULT_HOLD,
    IEMS_PF_REASON_FAULT_RECOVERED
} iems_pf_reason_t;

typedef struct {
    int en_pf_ctrl;      /* v3 EnPFCtrl; only 1 enables */
    int reactive_mode;   /* 0 = no output, 1 = PF, 2 = dynamic Q */
    double pf_target;    /* signed; sign selects the Q direction */
    double pf_deadband;  /* carried for later slices; not used here */
    double p_min_kw;     /* below this |P| the PF number is meaningless */
    int start_delay_s;   /* deviation must persist this long */
    int hold_s;          /* carried for later slices; not used here */
    int control_cycle_s; /* caller cadence; not used for elapsed time */
} iems_pf_cfg_t;

typedef struct {
    uint64_t now_ms; /* caller clock; the evaluator never reads one */
    double p_kw;
    double q_kvar;
    double pf;
    bool meter_online;
} iems_pf_input_t;

typedef struct {
    iems_pf_status_t state;
    /* 0 = not timing; otherwise the deviation start time + 1, so a start at
     * now_ms == 0 stays distinguishable from "not timing". At
     * now_ms == UINT64_MAX it saturates instead of wrapping to 0; the delay
     * then cannot elapse because the caller clock cannot advance further. */
    uint64_t timer;
    double q_last;    /* last emitted q_target (held by ENABLED/IN_BAND/WAITING_START) */
    uint64_t last_ts; /* caller clock at this evaluation */
} iems_pf_state_t;

typedef struct {
    double q_target_kvar; /* actual command; see the state table above */
    iems_pf_status_t state;
    iems_pf_reason_t reason;
    iems_pf_state_t next_state;
} iems_pf_decision_t;

/* Returns a fresh DISABLED state. */
iems_pf_state_t iems_pf_state_init(void);

/* Pure evaluation; next_state is the caller's to carry. NULL cfg/input or an
 * out-of-range carried state returns DISABLED with 0 and BAD_CONFIG. */
iems_pf_decision_t iems_pf_evaluate(const iems_pf_cfg_t *cfg, const iems_pf_state_t *state,
                                    const iems_pf_input_t *in);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_PF_DECISION_H */
