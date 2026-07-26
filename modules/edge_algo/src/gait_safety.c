/**
 * @file gait_safety.c
 * @brief Multimodal Gait Analysis & Fall Hazard Active Safety Lock Implementation
 */

#include "gait_safety.h"
#include <string.h>
#include <math.h>

int gait_safety_init(gait_safety_status_t *status) {
    if (!status) {
        return -1;
    }

    memset(status, 0, sizeof(gait_safety_status_t));
    return 0;
}

int gait_safety_eval(const gait_sensory_input_t *input, gait_safety_status_t *status) {
    if (!input || !status) {
        return -1;
    }

    /* CoP displacement magnitude */
    float cop_dist = sqrtf(input->cop_x_cm * input->cop_x_cm + input->cop_y_cm * input->cop_y_cm);

    /* Risk Score heuristic calculation */
    float score = 0.0f;
    score += (cop_dist / 15.0f) * 0.3f;
    score += (fabsf(input->body_tilt_deg) / 45.0f) * 0.4f;
    score += (fabsf(input->tilt_velocity) / 120.0f) * 0.3f;

    if (score > 1.0f) score = 1.0f;
    status->hazard_score = score;

    /* Fall Hazard Thresholds: Tilt > 25 deg or tilt velocity > 80 deg/s or CoP > 10cm */
    if (fabsf(input->body_tilt_deg) > 25.0f || fabsf(input->tilt_velocity) > 80.0f || cop_dist > 12.0f || input->harness_tension_n > 150.0f) {
        status->fall_hazard_detected = true;
        status->emergency_lock_triggered = true;
    } else {
        status->fall_hazard_detected = false;
        status->emergency_lock_triggered = false;
    }

    return 0;
}
