/**
 * @file iems_soc_keep.c
 * @brief P5-03c: pure backup SOC-keep strategy decisions.
 */

#include "iems_soc_keep.h"

#include <math.h>

#define SOC_KEEP_MIN_CONFIG_POWER_KW 5.0
#define SOC_KEEP_DEFAULT_MARGIN_PCT 5.0
#define SOC_PCT_MIN 0.0
#define SOC_PCT_MAX 100.0

static double clamp_pct(double value) {
    if (value < SOC_PCT_MIN) return SOC_PCT_MIN;
    if (value > SOC_PCT_MAX) return SOC_PCT_MAX;
    return value;
}

static bool strategy_supported(int strategy_type) {
    return strategy_type == (int)IEMS_SOC_KEEP_STRATEGY_PERIOD_CTRL ||
           strategy_type == (int)IEMS_SOC_KEEP_STRATEGY_PV_STORAGE;
}

static bool cfg_valid(const iems_soc_keep_cfg_t *cfg) {
    if (!isfinite(cfg->active_soc_pct) || !isfinite(cfg->inactive_soc_pct) ||
        !isfinite(cfg->charge_power_kw) || !isfinite(cfg->max_charge_power_kw)) {
        return false;
    }
    if (cfg->active_soc_pct < SOC_PCT_MIN || cfg->active_soc_pct > SOC_PCT_MAX) {
        return false;
    }
    if (cfg->inactive_soc_pct < SOC_PCT_MIN || cfg->inactive_soc_pct > SOC_PCT_MAX) {
        return false;
    }
    if (cfg->active_soc_pct > cfg->inactive_soc_pct) return false;
    /* A zero delay would skip PENDING_ON/PENDING_OFF; the invariant needs them. */
    if (cfg->delay_on_ms == 0u || cfg->delay_off_ms == 0u) return false;
    return cfg->max_charge_power_kw >= 0.0;
}

/* v3 strategy_sockeep.c:65-70: <= 5 kW means "use the system maximum". */
static double charge_power_kw(const iems_soc_keep_cfg_t *cfg) {
    double power = cfg->charge_power_kw > SOC_KEEP_MIN_CONFIG_POWER_KW ? -fabs(cfg->charge_power_kw)
                                                                       : -cfg->max_charge_power_kw;
    return power < 0.0 ? power : 0.0;
}

/* v3 strategy_sockeep.c:120-125: get_sockeep_status stays true while the exit
 * delay runs, so PENDING_OFF keeps outputting the charge power. */
static bool status_charging(iems_soc_keep_status_t status) {
    return status == IEMS_SOC_KEEP_STATUS_ACTIVE || status == IEMS_SOC_KEEP_STATUS_PENDING_OFF;
}

static iems_soc_keep_state_t state_reset(const iems_soc_keep_state_t *state, uint64_t now_ms) {
    iems_soc_keep_state_t next = iems_soc_keep_state_init();
    next.last_now_ms = state && state->last_now_ms > now_ms ? state->last_now_ms : now_ms;
    next.first = false;
    return next;
}

/* v3 base/function.c:63-67: the switch happens when elapsed >= delay. */
static bool delay_elapsed(uint64_t now_ms, uint64_t since_ms, uint32_t delay_ms) {
    if (now_ms < since_ms) return false;
    return now_ms - since_ms >= (uint64_t)delay_ms;
}

iems_soc_keep_state_t iems_soc_keep_state_init(void) {
    iems_soc_keep_state_t state = {0};
    state.status = IEMS_SOC_KEEP_STATUS_INACTIVE;
    state.pending_target_status = IEMS_SOC_KEEP_STATUS_INACTIVE;
    state.first = true;
    return state;
}

/* Derived mirror so the state shape keeps pending_target_status without a
 * second source of truth: the four-state status already encodes the target. */
static iems_soc_keep_status_t pending_target_of(iems_soc_keep_status_t status) {
    if (status == IEMS_SOC_KEEP_STATUS_PENDING_ON) return IEMS_SOC_KEEP_STATUS_ACTIVE;
    if (status == IEMS_SOC_KEEP_STATUS_PENDING_OFF) return IEMS_SOC_KEEP_STATUS_INACTIVE;
    return status;
}

bool iems_soc_keep_resolve_thresholds(double active_soc_pct, double margin_pct,
                                      double *out_active_pct, double *out_inactive_pct) {
    if (!out_active_pct || !out_inactive_pct) return false;
    if (!isfinite(active_soc_pct) || !isfinite(margin_pct)) return false;
    if (active_soc_pct < SOC_PCT_MIN) return false;

    double margin = margin_pct > 0.0 ? margin_pct : SOC_KEEP_DEFAULT_MARGIN_PCT;
    *out_active_pct = clamp_pct(active_soc_pct);
    *out_inactive_pct = clamp_pct(active_soc_pct + margin);
    return true;
}

iems_soc_keep_decision_t iems_soc_keep_evaluate(const iems_soc_keep_cfg_t *cfg,
                                                const iems_soc_keep_state_t *state,
                                                const iems_soc_keep_input_t *in) {
    iems_soc_keep_decision_t decision = {0};
    decision.reason = IEMS_SOC_KEEP_REASON_BAD_CONFIG;

    if (!cfg || !in || !isfinite(in->soc_pct) || !cfg_valid(cfg)) {
        decision.next_state = state_reset(state, in ? in->now_ms : 0u);
        return decision;
    }

    iems_soc_keep_state_t st = state ? *state : iems_soc_keep_state_init();

    /* A4 outranks A5: a disabled or unsupported strategy is not evaluating,
     * so a backwards clock must not mask the reset to INACTIVE. */
    if (!cfg->enable || !in->enabled) {
        decision.reason = IEMS_SOC_KEEP_REASON_DISABLED;
        decision.next_state = state_reset(&st, in->now_ms);
        return decision;
    }
    if (!strategy_supported(in->strategy_type)) {
        decision.reason = IEMS_SOC_KEEP_REASON_UNSUPPORTED;
        decision.next_state = state_reset(&st, in->now_ms);
        return decision;
    }

    if (st.first) {
        st.last_now_ms = in->now_ms;
        st.first = false;
    } else if (in->now_ms < st.last_now_ms) {
        decision.reason = IEMS_SOC_KEEP_REASON_STALE;
        decision.next_state = st;
        decision.active = status_charging(st.status);
        decision.target_power_kw = decision.active ? charge_power_kw(cfg) : 0.0;
        return decision;
    } else {
        st.last_now_ms = in->now_ms;
    }

    iems_soc_keep_reason_t reason;
    uint64_t pending_since = st.pending_since_ms;
    switch (st.status) {
        case IEMS_SOC_KEEP_STATUS_INACTIVE:
        case IEMS_SOC_KEEP_STATUS_PENDING_ON: {
            bool below = in->soc_pct < cfg->active_soc_pct;
            if (st.status == IEMS_SOC_KEEP_STATUS_INACTIVE) {
                if (!below) {
                    reason = IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE;
                } else {
                    st.status = IEMS_SOC_KEEP_STATUS_PENDING_ON;
                    pending_since = in->now_ms;
                    reason = IEMS_SOC_KEEP_REASON_SOC_BELOW_ACTIVE;
                }
            } else if (!below) {
                st.status = IEMS_SOC_KEEP_STATUS_INACTIVE;
                pending_since = 0;
                reason = IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE;
            } else if (delay_elapsed(in->now_ms, st.pending_since_ms, cfg->delay_on_ms)) {
                st.status = IEMS_SOC_KEEP_STATUS_ACTIVE;
                pending_since = 0;
                reason = IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE;
            } else {
                reason = IEMS_SOC_KEEP_REASON_PENDING_ON;
            }
            break;
        }
        case IEMS_SOC_KEEP_STATUS_ACTIVE:
        case IEMS_SOC_KEEP_STATUS_PENDING_OFF: {
            bool above = in->soc_pct >= cfg->inactive_soc_pct;
            if (st.status == IEMS_SOC_KEEP_STATUS_ACTIVE) {
                if (!above) {
                    reason = IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE;
                } else {
                    st.status = IEMS_SOC_KEEP_STATUS_PENDING_OFF;
                    pending_since = in->now_ms;
                    reason = IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE;
                }
            } else if (!above) {
                st.status = IEMS_SOC_KEEP_STATUS_ACTIVE;
                pending_since = 0;
                reason = IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE;
            } else if (delay_elapsed(in->now_ms, st.pending_since_ms, cfg->delay_off_ms)) {
                st.status = IEMS_SOC_KEEP_STATUS_INACTIVE;
                pending_since = 0;
                reason = IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE;
            } else {
                reason = IEMS_SOC_KEEP_REASON_PENDING_OFF;
            }
            break;
        }
        default:
            st.status = IEMS_SOC_KEEP_STATUS_INACTIVE;
            pending_since = 0;
            reason = IEMS_SOC_KEEP_REASON_BAD_CONFIG;
            break;
    }

    st.pending_since_ms = pending_since;
    st.pending_target_status = pending_target_of(st.status);
    decision.reason = reason;
    decision.next_state = st;
    decision.active = status_charging(st.status);
    decision.target_power_kw = decision.active ? charge_power_kw(cfg) : 0.0;
    return decision;
}
