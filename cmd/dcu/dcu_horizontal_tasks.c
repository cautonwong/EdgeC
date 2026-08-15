/**
 * @file dcu_horizontal_tasks.c
 * @brief Production DCU Horizontal Tasks Engine Implementation
 */

#include "dcu_horizontal_tasks.h"
#include <string.h>
#include <math.h>

void dcu_horizontal_init(dcu_horizontal_engine_t* engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(dcu_horizontal_engine_t));
    engine->vvc_state.target_voltage_v = 220.0;
}

int dcu_horizontal_compute_spatial_loss(dcu_horizontal_engine_t* engine, uint32_t branch_id,
                                        const char* branch_name, double input_kwh,
                                        double consumed_kwh, uint32_t meters) {
    if (!engine || branch_id == 0 || branch_id > DCU_MAX_BRANCHES) return -1;

    uint32_t idx = branch_id - 1;
    dcu_branch_spatial_loss_t* b = &engine->branches[idx];
    b->branch_id = branch_id;
    if (branch_name) {
        strncpy(b->branch_name, branch_name, sizeof(b->branch_name) - 1);
    }
    b->branch_input_energy_kwh = input_kwh;
    b->branch_consumed_kwh = consumed_kwh;
    b->meter_count = meters;

    if (input_kwh > 0.001) {
        b->branch_loss_rate_pct = ((input_kwh - consumed_kwh) / input_kwh) * 100.0;
    } else {
        b->branch_loss_rate_pct = 0.0;
    }

    // Flag abnormal if loss rate > 8% or < 0%
    b->is_abnormal = (b->branch_loss_rate_pct > 8.0 || b->branch_loss_rate_pct < 0.0);

    if (branch_id > engine->branch_count) {
        engine->branch_count = branch_id;
    }

    return 0;
}

int dcu_horizontal_evaluate_phase_balancing(dcu_horizontal_engine_t* engine,
                                            double ia, double ib, double ic,
                                            uint32_t heaviest_meter_id,
                                            const char* heaviest_meter_addr,
                                            uint8_t meter_phase, double meter_current,
                                            uint64_t now_ms) {
    if (!engine) return -1;

    double currents[3] = {ia, ib, ic};
    double i_avg = (ia + ib + ic) / 3.0;
    if (i_avg < 0.01) return 0; // Negligible current

    int max_idx = 0;
    int min_idx = 0;
    for (int i = 1; i < 3; i++) {
        if (currents[i] > currents[max_idx]) max_idx = i;
        if (currents[i] < currents[min_idx]) min_idx = i;
    }

    double unbalance_pct = (fabs(currents[max_idx] - i_avg) / i_avg) * 100.0;

    // Trigger commutation if unbalance > 15% and meter is on the heavily loaded phase
    if (unbalance_pct > 15.0 && (meter_phase - 1) == max_idx && max_idx != min_idx) {
        engine->last_commutation.target_meter_dev_id = heaviest_meter_id;
        if (heaviest_meter_addr) {
            strncpy(engine->last_commutation.target_meter_addr, heaviest_meter_addr,
                    sizeof(engine->last_commutation.target_meter_addr) - 1);
        }
        engine->last_commutation.from_phase = (uint8_t)(max_idx + 1);
        engine->last_commutation.to_phase = (uint8_t)(min_idx + 1);
        engine->last_commutation.expected_shift_current_a = meter_current;
        engine->last_commutation.decision_timestamp_ms = now_ms;
        return 1; // Commutation recommended
    }

    return 0;
}

int dcu_horizontal_compute_volt_var(dcu_horizontal_engine_t* engine, double head_u,
                                    double tail_u, double total_pv_kw) {
    if (!engine) return -1;

    dcu_volt_var_state_t* v = &engine->vvc_state;
    v->feeder_head_voltage_v = head_u;
    v->feeder_tail_voltage_v = tail_u;
    v->total_pv_active_kw = total_pv_kw;

    double delta_u_tail = v->target_voltage_v - tail_u; // e.g. 220 - 205 = 15V drop

    // If tail voltage dropped by > 5% (below 209V)
    if (tail_u < 209.0) {
        v->is_vvc_active = true;
        // Inject capacitive reactive power: Q_inj = delta_U * sensitivity_factor
        v->required_q_inject_kvar = delta_u_tail * 2.5; // 2.5 kVAR per volt drop
    } else if (head_u > 235.0) { // Overvoltage due to heavy solar reverse flow
        v->is_vvc_active = true;
        // Absorb inductive reactive power
        v->required_q_inject_kvar = -(head_u - 235.0) * 3.0;
    } else {
        v->is_vvc_active = false;
        v->required_q_inject_kvar = 0.0;
    }

    return 0;
}
