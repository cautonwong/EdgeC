/**
 * @file iems_pf_decision.c
 * @brief P2-03b: pure PCC power-factor decision module.
 */

#include "iems_pf_decision.h"

#include <math.h>

#define PF_TARGET_MIN_ABS 0.8
#define PF_TARGET_MAX_ABS 1.0
#define PF_START_DELAY_MIN_S 0
#define PF_START_DELAY_MAX_S 60
#define PF_Q_TOL_KVAR 0.30

static double resolve_pf_target(double target) {
    if (!isfinite(target)) return IEMS_PF_DEFAULT_PF_TARGET;
    double magnitude = fabs(target);
    if (magnitude < PF_TARGET_MIN_ABS || magnitude > PF_TARGET_MAX_ABS) {
        return target < 0.0 ? -IEMS_PF_DEFAULT_PF_TARGET : IEMS_PF_DEFAULT_PF_TARGET;
    }
    return target;
}

static double resolve_p_min(double p_min_kw) {
    return (isfinite(p_min_kw) && p_min_kw > 0.0) ? p_min_kw : IEMS_PF_DEFAULT_P_MIN_KW;
}

static uint64_t resolve_start_delay_ms(int start_delay_s) {
    int seconds = start_delay_s;
    if (seconds < PF_START_DELAY_MIN_S || seconds > PF_START_DELAY_MAX_S) {
        seconds = IEMS_PF_DEFAULT_START_DELAY_S;
    }
    return (uint64_t)seconds * 1000u;
}

static iems_pf_state_t state_reset(iems_pf_status_t status, uint64_t now_ms) {
    iems_pf_state_t state = iems_pf_state_init();
    state.state = status;
    state.last_ts = now_ms;
    return state;
}

static iems_pf_decision_t decision_make(iems_pf_state_t state, double q_target,
                                        iems_pf_reason_t reason) {
    state.q_last = q_target;
    iems_pf_decision_t decision = {0};
    decision.q_target_kvar = q_target;
    decision.state = state.state;
    decision.reason = reason;
    decision.next_state = state;
    return decision;
}

iems_pf_state_t iems_pf_state_init(void) {
    iems_pf_state_t state = {0};
    state.state = IEMS_PF_STATE_DISABLED;
    return state;
}

iems_pf_decision_t iems_pf_evaluate(const iems_pf_cfg_t *cfg, const iems_pf_state_t *state,
                                    const iems_pf_input_t *in) {
    if (!cfg || !in) {
        iems_pf_state_t st = state ? *state : iems_pf_state_init();
        st.state = IEMS_PF_STATE_DISABLED;
        st.timer = 0u;
        st.last_ts = in ? in->now_ms : (state ? state->last_ts : 0u);
        return decision_make(st, 0.0, IEMS_PF_REASON_BAD_CONFIG);
    }

    iems_pf_state_t st = state ? *state : iems_pf_state_init();
    st.last_ts = in->now_ms;

    if ((int)st.state < (int)IEMS_PF_STATE_DISABLED || (int)st.state > (int)IEMS_PF_STATE_FAULT) {
        return decision_make(state_reset(IEMS_PF_STATE_DISABLED, in->now_ms), 0.0,
                             IEMS_PF_REASON_BAD_CONFIG);
    }

    if (cfg->en_pf_ctrl != 1) {
        return decision_make(state_reset(IEMS_PF_STATE_DISABLED, in->now_ms), 0.0,
                             IEMS_PF_REASON_DISABLED);
    }
    if (cfg->reactive_mode == 0) {
        return decision_make(state_reset(IEMS_PF_STATE_DISABLED, in->now_ms), 0.0,
                             IEMS_PF_REASON_NO_OUTPUT);
    }
    if (!in->meter_online) {
        return decision_make(state_reset(IEMS_PF_STATE_FAULT, in->now_ms), 0.0,
                             IEMS_PF_REASON_METER_OFFLINE);
    }
    if (!isfinite(in->pf) || !isfinite(in->p_kw) || !isfinite(in->q_kvar)) {
        return decision_make(state_reset(IEMS_PF_STATE_FAULT, in->now_ms), 0.0,
                             IEMS_PF_REASON_BAD_MEASUREMENT);
    }
    if (fabs(in->pf) > 1.0) {
        return decision_make(state_reset(IEMS_PF_STATE_FAULT, in->now_ms), 0.0,
                             IEMS_PF_REASON_PF_OUT_OF_RANGE);
    }

    if (st.state == IEMS_PF_STATE_DISABLED) {
        st.state = IEMS_PF_STATE_STANDBY;
        st.timer = 0u;
        return decision_make(st, st.q_last, IEMS_PF_REASON_ENABLED);
    }
    if (st.state == IEMS_PF_STATE_FAULT) {
        return decision_make(state_reset(IEMS_PF_STATE_STANDBY, in->now_ms), 0.0,
                             IEMS_PF_REASON_FAULT_RECOVERED);
    }

    double p_min_kw = resolve_p_min(cfg->p_min_kw);
    if (fabs(in->p_kw) < p_min_kw) {
        return decision_make(state_reset(IEMS_PF_STATE_STANDBY, in->now_ms), 0.0,
                             IEMS_PF_REASON_LOW_POWER);
    }

    double pf_target = resolve_pf_target(cfg->pf_target);
    double sigma = pf_target >= 0.0 ? 1.0 : -1.0;
    double pf_abs = fmin(fabs(pf_target), 1.0);
    double qt = fabs(in->p_kw) * tan(acos(pf_abs));
    double q_tol = fmin(0.5 * qt, PF_Q_TOL_KVAR);
    double q_edge = qt - q_tol;
    double q_al = in->q_kvar * sigma;

    if (fabs(q_al) <= qt) {
        st.state = IEMS_PF_STATE_STANDBY;
        st.timer = 0u;
        return decision_make(st, st.q_last, IEMS_PF_REASON_IN_BAND);
    }

    if (st.state == IEMS_PF_STATE_STANDBY) {
        if (st.timer == 0u) {
            st.timer = in->now_ms == UINT64_MAX ? UINT64_MAX : in->now_ms + 1u;
        }
        uint64_t start = st.timer - 1u;
        uint64_t elapsed = in->now_ms > start ? in->now_ms - start : 0u;
        uint64_t delay_ms = resolve_start_delay_ms(cfg->start_delay_s);
        if (elapsed < delay_ms) {
            return decision_make(st, st.q_last, IEMS_PF_REASON_WAITING_START);
        }
        st.state = IEMS_PF_STATE_RUNNING;
    }

    st.timer = 0u;
    double q_ref = q_al > q_edge ? q_edge : (q_al < -q_edge ? -q_edge : q_al);
    return decision_make(st, q_ref * sigma, IEMS_PF_REASON_RUNNING);
}
