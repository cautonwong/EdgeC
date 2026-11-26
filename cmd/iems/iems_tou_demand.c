/**
 * @file iems_tou_demand.c
 * @brief P5-03b/c: pure TOU/price and demand strategy decisions.
 */

#include "iems_tou_demand.h"

#define IEMS_TD_DEFAULT_WINDOW_MIN 15u
#define IEMS_TD_SECONDS_PER_DAY 86400u

static bool period_matches(const iems_td_period_t *period, uint32_t second_of_day) {
    if (period->start_sec >= IEMS_TD_SECONDS_PER_DAY ||
        period->end_sec > IEMS_TD_SECONDS_PER_DAY) {
        return false;
    }
    /* v3 jsonct.c:598: a 00:00-00:00 slot is an unused placeholder. */
    if (period->start_sec == 0u && period->end_sec == 0u) return false;
    if (period->end_sec > period->start_sec) {
        return second_of_day > period->start_sec && second_of_day <= period->end_sec;
    }
    return second_of_day > period->start_sec || second_of_day <= period->end_sec;
}

int iems_td_find_period(const iems_td_period_t *periods, size_t count, uint32_t second_of_day) {
    if (!periods) return -1;
    for (size_t i = 0; i < count; i++) {
        if (period_matches(&periods[i], second_of_day)) return (int)i;
    }
    return -1;
}

bool iems_td_price_lookup(const iems_td_price_table_t *table, uint8_t month,
                          iems_td_price_class_t price_class, double *out_rate) {
    if (!table || !table->rows || !out_rate) return false;
    if (month < 1u || (size_t)month > table->count) return false;

    const iems_td_price_row_t *row = &table->rows[month - 1u];
    switch (price_class) {
        case IEMS_TD_PRICE_TOP:
            *out_rate = row->top;
            return true;
        case IEMS_TD_PRICE_PEAK:
            *out_rate = row->peak;
            return true;
        case IEMS_TD_PRICE_FLAT:
            *out_rate = row->flat;
            return true;
        case IEMS_TD_PRICE_VALLEY:
            *out_rate = row->valley;
            return true;
        case IEMS_TD_PRICE_NONE:
        default:
            return false;
    }
}

void iems_td_window_add(double *sum_kw, uint32_t *samples, double power_kw, uint32_t max_samples) {
    if (!sum_kw || !samples) return;
    if (power_kw > 0.0) *sum_kw += power_kw;
    (*samples)++;
    if (max_samples > 0u && *samples > max_samples) {
        *sum_kw /= 2.0;
        *samples /= 2u;
    }
}

double iems_td_window_average(double sum_kw, uint32_t samples) {
    if (samples == 0u) return 0.0;
    return sum_kw / (double)samples;
}

static int8_t action_from_power(double power_kw) {
    if (power_kw > 0.0) return (int8_t)IEMS_TD_ACTION_DISCHARGE;
    if (power_kw < 0.0) return (int8_t)IEMS_TD_ACTION_CHARGE;
    return (int8_t)IEMS_TD_ACTION_STANDBY;
}

double iems_td_demand_raw(const iems_td_demand_config_t *cfg, int8_t prev_action,
                          const iems_td_demand_input_t *in, int8_t *next_action) {
    int8_t next = (prev_action == (int8_t)IEMS_TD_ACTION_DISCHARGE ||
                   prev_action == (int8_t)IEMS_TD_ACTION_CHARGE)
                      ? prev_action
                      : (int8_t)IEMS_TD_ACTION_STANDBY;
    double raw = 0.0;

    if (!cfg || !in) {
        if (next_action) *next_action = next;
        return 0.0;
    }

    switch (cfg->mode) {
        case IEMS_TD_DEMAND_LIMIT: {
            /* EdgeC coordinator: shave the larger of the instant and window demand. */
            double demand = in->grid_kw > in->demand_kw ? in->grid_kw : in->demand_kw;
            raw = demand - cfg->limit_kw;
            if (raw < 0.0) raw = 0.0;
            next = (int8_t)(raw > 0.0 ? IEMS_TD_ACTION_DISCHARGE : IEMS_TD_ACTION_STANDBY);
            break;
        }
        case IEMS_TD_DEMAND_THRESHOLD: {
            /* v3 fixed demand: the latched flag is only replaced when the
             * opposite threshold is crossed (strategy_demand_fixed.c:356-371). */
            if (in->grid_kw > cfg->limit_kw) {
                next = (int8_t)IEMS_TD_ACTION_DISCHARGE;
            } else if (in->grid_kw < cfg->lower_kw) {
                next = (int8_t)IEMS_TD_ACTION_CHARGE;
            }
            if (next == (int8_t)IEMS_TD_ACTION_DISCHARGE) {
                raw = in->grid_kw - cfg->limit_kw;
                if (raw < 0.0) raw = 0.0;
            } else if (next == (int8_t)IEMS_TD_ACTION_CHARGE && cfg->charge_enabled) {
                raw = in->grid_kw - cfg->lower_kw;
                if (raw > 0.0) raw = 0.0;
            }
            break;
        }
        case IEMS_TD_DEMAND_WINDOW: {
            /* v3 dynamic demand: (demand - target) * elapsed minutes
             * (strategy_demand_ctrl.c:1826-1833). */
            uint32_t window_min = cfg->window_min;
            if (window_min == 0u || window_min > 60u) {
                window_min = IEMS_TD_DEFAULT_WINDOW_MIN;
            }
            uint32_t elapsed = in->window_elapsed_min;
            if (elapsed < 1u) elapsed = 1u;
            if (elapsed > window_min) elapsed = window_min;

            if (in->demand_kw >= cfg->limit_kw) {
                raw = (in->demand_kw - cfg->limit_kw) * (double)(elapsed - 1u);
            } else if (cfg->charge_enabled && in->demand_kw < cfg->charge_threshold_kw) {
                raw = (in->demand_kw - cfg->charge_threshold_kw) * (double)(elapsed - 1u);
            }
            next = action_from_power(raw);
            break;
        }
        case IEMS_TD_DEMAND_OFF:
        default:
            next = (int8_t)IEMS_TD_ACTION_STANDBY;
            break;
    }

    if (next_action) *next_action = next;
    return raw;
}

iems_td_decision_t iems_td_evaluate(const iems_td_config_t *cfg, const iems_td_state_t *state,
                                    const iems_td_input_t *in) {
    iems_td_decision_t decision = {0};
    decision.reason = IEMS_TD_REASON_INVALID;
    decision.action = IEMS_TD_ACTION_STANDBY;
    if (!cfg || !in) return decision;

    decision.next_state = state ? *state : (iems_td_state_t){0};

    double target = 0.0;
    double soc_floor = cfg->soc_min_pct;
    double soc_ceiling = cfg->soc_max_pct;

    int period_index =
        iems_td_find_period(cfg->periods, cfg->period_count, in->second_of_day);
    if (period_index >= 0) {
        const iems_td_period_t *period = &cfg->periods[period_index];
        target = period->target_power_kw;
        if (period->soc_min_pct > soc_floor) soc_floor = period->soc_min_pct;
        if (period->soc_max_pct < soc_ceiling) soc_ceiling = period->soc_max_pct;

        if (target > 0.0) {
            decision.reason = IEMS_TD_REASON_TOU_DISCHARGE;
        } else if (target < 0.0) {
            decision.reason = IEMS_TD_REASON_TOU_CHARGE;
        } else {
            decision.reason = IEMS_TD_REASON_NONE;
        }

        if (period->price_class != IEMS_TD_PRICE_NONE) {
            decision.price_valid = iems_td_price_lookup(
                &cfg->price_table, in->month, period->price_class, &decision.price_cny_per_kwh);
        }
    } else {
        decision.reason = IEMS_TD_REASON_NO_PERIOD;
    }

    if (cfg->demand.mode != IEMS_TD_DEMAND_OFF) {
        iems_td_demand_input_t demand_in = {.grid_kw = in->grid_kw,
                                            .demand_kw = in->demand_kw,
                                            .window_elapsed_min = in->window_elapsed_min};
        int8_t next_action = decision.next_state.demand_action;
        double demand_target = iems_td_demand_raw(&cfg->demand, next_action, &demand_in,
                                                  &next_action);
        decision.next_state.demand_action = next_action;
        decision.demand_active = demand_target != 0.0;
        /* Demand raises discharge freely and deepens a non-discharging baseline
         * toward charge; it never flips an active TOU discharge into charge. */
        if (demand_target > 0.0 && demand_target > target) {
            target = demand_target;
            decision.reason = IEMS_TD_REASON_DEMAND_DISCHARGE;
        } else if (demand_target < 0.0 && target <= 0.0 && demand_target < target) {
            target = demand_target;
            decision.reason = IEMS_TD_REASON_DEMAND_CHARGE;
        }
    } else {
        decision.next_state.demand_action = (int8_t)IEMS_TD_ACTION_STANDBY;
    }

    if (target > 0.0 && in->soc_pct <= soc_floor) {
        target = 0.0;
        decision.reason = IEMS_TD_REASON_SOC_LIMIT;
    } else if (target < 0.0 && in->soc_pct >= soc_ceiling) {
        target = 0.0;
        decision.reason = IEMS_TD_REASON_SOC_LIMIT;
    }

    double max_charge = cfg->max_charge_kw > 0.0 ? cfg->max_charge_kw : 0.0;
    double max_discharge = cfg->max_discharge_kw > 0.0 ? cfg->max_discharge_kw : 0.0;
    if (target > max_discharge) target = max_discharge;
    if (target < -max_charge) target = -max_charge;

    decision.target_power_kw = target;
    decision.action = (iems_td_action_t)action_from_power(target);
    return decision;
}
