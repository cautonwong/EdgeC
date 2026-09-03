/**
 * @file dcu_power_quality.c
 * @brief DCU Power Quality Analytics & Three-Phase Unbalance Phase-Switching Optimization Implementation
 */

#include "dcu_power_quality.h"
#include <string.h>
#include <math.h>

void dcu_power_quality_init(dcu_power_quality_engine_t *engine, double nominal_voltage, double unbalance_threshold_pct) {
    if (!engine) return;
    memset(engine, 0, sizeof(dcu_power_quality_engine_t));
    engine->nominal_voltage = (nominal_voltage > 0.0) ? nominal_voltage : 220.0;
    engine->unbalance_threshold_pct = (unbalance_threshold_pct > 0.0) ? unbalance_threshold_pct : 15.0;
}

double dcu_calc_three_phase_unbalance(double a, double b, double c) {
    double avg = (a + b + c) / 3.0;
    if (avg < 1e-6) return 0.0;

    double diff_a = fabs(a - avg);
    double diff_b = fabs(b - avg);
    double diff_c = fabs(c - avg);

    double max_diff = diff_a;
    if (diff_b > max_diff) max_diff = diff_b;
    if (diff_c > max_diff) max_diff = diff_c;

    return (max_diff / avg) * 100.0;
}

int dcu_solve_phase_switch_plan(dcu_power_quality_engine_t *engine,
                                const dcu_switchable_branch_t *branches,
                                uint8_t branch_count,
                                dcu_phase_switch_plan_t *out_plan) {
    if (!engine || !out_plan) return -1;
    memset(out_plan, 0, sizeof(dcu_phase_switch_plan_t));

    double ia = engine->ia;
    double ib = engine->ib;
    double ic = engine->ic;

    double init_unbalance = dcu_calc_three_phase_unbalance(ia, ib, ic);
    out_plan->initial_unbalance_pct = init_unbalance;
    out_plan->projected_unbalance_pct = init_unbalance;

    if (init_unbalance <= engine->unbalance_threshold_pct || branch_count == 0 || !branches) {
        out_plan->is_action_recommended = false;
        return 0;
    }

    // 寻找当前负荷最大相与最小相
    double cur[3] = {ia, ib, ic};
    pq_phase_t max_phase = PQ_PHASE_A;
    pq_phase_t min_phase = PQ_PHASE_A;

    for (int p = 1; p < 3; p++) {
        if (cur[p] > cur[max_phase]) max_phase = (pq_phase_t)p;
        if (cur[p] < cur[min_phase]) min_phase = (pq_phase_t)p;
    }

    // 遍历可换相支路，寻找最优迁移单相负荷
    int best_branch_idx = -1;
    double best_projected_unbalance = init_unbalance;

    for (uint8_t i = 0; i < branch_count; i++) {
        if (branches[i].current_phase == max_phase && branches[i].current_amps > 0.0) {
            double test_cur[3] = {ia, ib, ic};
            test_cur[max_phase] -= branches[i].current_amps;
            test_cur[min_phase] += branches[i].current_amps;

            double test_unbalance = dcu_calc_three_phase_unbalance(test_cur[0], test_cur[1], test_cur[2]);
            if (test_unbalance < best_projected_unbalance) {
                best_projected_unbalance = test_unbalance;
                best_branch_idx = i;
            }
        }
    }

    if (best_branch_idx >= 0 && best_projected_unbalance < init_unbalance) {
        out_plan->action_count = 1;
        out_plan->actions[0].meter_id = branches[best_branch_idx].meter_id;
        out_plan->actions[0].from_phase = max_phase;
        out_plan->actions[0].to_phase = min_phase;
        out_plan->actions[0].migrated_current_amps = branches[best_branch_idx].current_amps;
        out_plan->projected_unbalance_pct = best_projected_unbalance;
        out_plan->is_action_recommended = true;
    }

    return 0;
}

int dcu_analyze_harmonics_thd(double fundamental,
                              const double *harmonic_components,
                              dcu_harmonic_analysis_t *out_analysis) {
    if (!out_analysis || fundamental < 1e-6 || !harmonic_components) return -1;
    memset(out_analysis, 0, sizeof(dcu_harmonic_analysis_t));

    out_analysis->fundamental_val = fundamental;

    double sum_sq_all = 0.0;
    double sum_sq_odd = 0.0;
    double sum_sq_even = 0.0;

    for (uint8_t k = 2; k < DCU_MAX_HARMONICS; k++) {
        double val = harmonic_components[k];
        out_analysis->harmonic_vals[k] = val;
        out_analysis->harmonic_pct[k] = (val / fundamental) * 100.0;

        double sq = val * val;
        sum_sq_all += sq;
        if ((k % 2) != 0) {
            sum_sq_odd += sq; // 3, 5, 7, 9 ... 31
        } else {
            sum_sq_even += sq; // 2, 4, 6, 8 ... 30
        }
    }

    out_analysis->thd_total_pct = (sqrt(sum_sq_all) / fundamental) * 100.0;
    out_analysis->thd_odd_pct   = (sqrt(sum_sq_odd) / fundamental) * 100.0;
    out_analysis->thd_even_pct  = (sqrt(sum_sq_even) / fundamental) * 100.0;

    return 0;
}

int dcu_detect_voltage_transient(dcu_power_quality_engine_t *engine,
                                 double current_voltage,
                                 uint64_t timestamp_ms,
                                 dcu_transient_event_t *out_event) {
    if (!engine || !out_event) return -1;

    double un = (engine->nominal_voltage > 0.0) ? engine->nominal_voltage : 220.0;
    double ratio = current_voltage / un;

    pq_transient_event_type_t current_type = PQ_EVENT_NONE;
    if (ratio < 0.1) {
        current_type = PQ_EVENT_INTERRUPTION;
    } else if (ratio < 0.9) {
        current_type = PQ_EVENT_VOLTAGE_SAG;
    } else if (ratio > 1.1) {
        current_type = PQ_EVENT_VOLTAGE_SWELL;
    }

    // 状态机处理
    if (!engine->transient_event.is_active) {
        if (current_type != PQ_EVENT_NONE) {
            // 新暂态事件产生
            engine->transient_event.is_active = true;
            engine->transient_event.event_type = current_type;
            engine->transient_event.start_timestamp_ms = timestamp_ms;
            engine->transient_event.nominal_voltage = un;
            engine->transient_event.extreme_voltage = current_voltage;
            engine->transient_event.duration_ms = 0;
            *out_event = engine->transient_event;
        } else {
            memset(out_event, 0, sizeof(dcu_transient_event_t));
        }
    } else {
        // 事件持续中
        engine->transient_event.duration_ms = timestamp_ms - engine->transient_event.start_timestamp_ms;
        if (engine->transient_event.event_type == PQ_EVENT_VOLTAGE_SAG && current_voltage < engine->transient_event.extreme_voltage) {
            engine->transient_event.extreme_voltage = current_voltage;
        } else if (engine->transient_event.event_type == PQ_EVENT_VOLTAGE_SWELL && current_voltage > engine->transient_event.extreme_voltage) {
            engine->transient_event.extreme_voltage = current_voltage;
        }

        if (current_type == PQ_EVENT_NONE) {
            // 恢复正常，结算事件
            engine->transient_event.is_active = false;
            *out_event = engine->transient_event;
        } else {
            *out_event = engine->transient_event;
        }
    }

    return 0;
}
