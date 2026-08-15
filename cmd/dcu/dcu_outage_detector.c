/**
 * @file dcu_outage_detector.c
 * @brief Production DCU Low-Voltage Outage Detection & Last-Gasp Fault Location Engine Implementation
 */

#include "dcu_outage_detector.h"
#include <string.h>

void dcu_outage_detector_init(dcu_outage_detector_t* detector) {
    if (!detector) return;
    memset(detector, 0, sizeof(dcu_outage_detector_t));
    detector->status = OUTAGE_STATUS_NORMAL;
    detector->supercap_voltage_mv = 5000;
}

dcu_outage_status_t dcu_outage_detector_evaluate(dcu_outage_detector_t* detector,
                                                 double ua, double ub, double uc,
                                                 double nominal_v, uint32_t supercap_mv,
                                                 uint64_t now_ms) {
    if (!detector) return OUTAGE_STATUS_NORMAL;

    detector->supercap_voltage_mv = supercap_mv;
    double threshold_v = (nominal_v > 0.0) ? (nominal_v * 0.20) : 44.0; // 20% Un

    int lost_phase_count = 0;
    if (ua < threshold_v) lost_phase_count++;
    if (ub < threshold_v) lost_phase_count++;
    if (uc < threshold_v) lost_phase_count++;

    dcu_outage_status_t prev_status = detector->status;

    if (lost_phase_count == 3) {
        detector->status = OUTAGE_STATUS_FULL_BLACKOUT;
        if (prev_status != OUTAGE_STATUS_FULL_BLACKOUT) {
            detector->outage_start_timestamp_ms = now_ms;
            detector->total_outage_events++;
            detector->is_last_gasp_sent = false;
        }
    } else if (lost_phase_count == 2) {
        detector->status = OUTAGE_STATUS_TWO_PHASE;
        if (prev_status == OUTAGE_STATUS_NORMAL) {
            detector->outage_start_timestamp_ms = now_ms;
            detector->total_outage_events++;
        }
    } else if (lost_phase_count == 1) {
        detector->status = OUTAGE_STATUS_SINGLE_PHASE;
        if (prev_status == OUTAGE_STATUS_NORMAL) {
            detector->outage_start_timestamp_ms = now_ms;
            detector->total_outage_events++;
        }
    } else {
        detector->status = OUTAGE_STATUS_NORMAL;
        if (prev_status != OUTAGE_STATUS_NORMAL) {
            detector->last_restoration_timestamp_ms = now_ms;
        }
    }

    return detector->status;
}
