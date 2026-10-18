/**
 * @file dcu_topology_phase_id.c
 * @brief HPLC Dual-Mode Carrier Topology & Phase Identification Solver Implementation (AEP-461)
 */

#include "dcu_topology_phase_id.h"

#include <math.h>
#include <string.h>

static double _circ_dist(double t1, double t2, double period) {
    double diff = fmod(fabs(t1 - t2), period);
    if (diff > period / 2.0) {
        diff = period - diff;
    }
    return diff;
}

dcu_phase_t dcu_hplc_identify_single_phase(double zero_crossing_offset_us, float *out_confidence) {
    double mod_offset = fmod(zero_crossing_offset_us, GRID_NOMINAL_PERIOD_US);
    if (mod_offset < 0.0) {
        mod_offset += GRID_NOMINAL_PERIOD_US;
    }

    double dist_a = _circ_dist(mod_offset, PHASE_A_REF_US, GRID_NOMINAL_PERIOD_US);
    double dist_b = _circ_dist(mod_offset, PHASE_B_REF_US, GRID_NOMINAL_PERIOD_US);
    double dist_c = _circ_dist(mod_offset, PHASE_C_REF_US, GRID_NOMINAL_PERIOD_US);

    dcu_phase_t best_phase = PHASE_UNKNOWN;
    double min_dist = dist_a;
    best_phase = PHASE_A;

    if (dist_b < min_dist) {
        min_dist = dist_b;
        best_phase = PHASE_B;
    }
    if (dist_c < min_dist) {
        min_dist = dist_c;
        best_phase = PHASE_C;
    }

    // 允许的过零点最大公差: 3333 us (half of 120 degrees)
    // 置信度模型: 1.0 - (min_dist / 3333.3)
    float conf = 1.0f - (float)(min_dist / (GRID_NOMINAL_PERIOD_US / 6.0));
    if (conf < 0.0f) conf = 0.0f;
    if (conf > 1.0f) conf = 1.0f;

    if (out_confidence) {
        *out_confidence = conf;
    }

    if (conf < 0.50f) {
        return PHASE_UNKNOWN;
    }

    return best_phase;
}

int dcu_hplc_solve_topology_and_phases(dcu_hplc_meter_sample_t *samples, uint32_t count,
                                       dcu_phase_id_summary_t *out_summary) {
    if (!samples || count == 0) return -1;

    dcu_phase_id_summary_t summary = {0};
    summary.total_meters = count;
    float total_conf = 0.0f;

    for (uint32_t i = 0; i < count; i++) {
        float conf = 0.0f;
        dcu_phase_t ph = dcu_hplc_identify_single_phase(samples[i].zero_crossing_us, &conf);
        samples[i].identified_phase = ph;
        samples[i].confidence = conf;
        total_conf += conf;

        switch (ph) {
            case PHASE_A:
                summary.phase_a_count++;
                break;
            case PHASE_B:
                summary.phase_b_count++;
                break;
            case PHASE_C:
                summary.phase_c_count++;
                break;
            default:
                summary.unknown_count++;
                break;
        }
    }

    summary.avg_confidence = total_conf / (float)count;
    if (out_summary) {
        *out_summary = summary;
    }
    return 0;
}
