#include "hrv_analyzer.h"
#include <math.h>
#include <string.h>

void hrv_analyzer_init(hrv_analyzer_t *hrv) {
    if (!hrv) return;
    memset(hrv, 0, sizeof(hrv_analyzer_t));
}

int hrv_analyzer_push_rr(hrv_analyzer_t *hrv, float rr_ms) {
    if (!hrv || rr_ms <= 0.0f) return -1;
    if (hrv->count < MAX_RR_INTERVALS) {
        hrv->rr_intervals_ms[hrv->count++] = rr_ms;
    } else {
        memmove(&hrv->rr_intervals_ms[0], &hrv->rr_intervals_ms[1], (MAX_RR_INTERVALS - 1) * sizeof(float));
        hrv->rr_intervals_ms[MAX_RR_INTERVALS - 1] = rr_ms;
    }
    return 0;
}

void hrv_analyzer_calculate(hrv_analyzer_t *hrv) {
    if (!hrv || hrv->count < 2) return;

    // Calculate mean RR
    float sum_rr = 0.0f;
    for (size_t i = 0; i < hrv->count; i++) {
        sum_rr += hrv->rr_intervals_ms[i];
    }
    float mean_rr = sum_rr / (float)hrv->count;

    // Calculate SDNN (Standard Deviation of NN)
    float var_sum = 0.0f;
    for (size_t i = 0; i < hrv->count; i++) {
        float diff = hrv->rr_intervals_ms[i] - mean_rr;
        var_sum += diff * diff;
    }
    hrv->sdnn_ms = sqrtf(var_sum / (float)hrv->count);

    // Calculate RMSSD (Root Mean Square of Successive Differences)
    float succ_diff_sq_sum = 0.0f;
    for (size_t i = 0; i < hrv->count - 1; i++) {
        float diff = hrv->rr_intervals_ms[i + 1] - hrv->rr_intervals_ms[i];
        succ_diff_sq_sum += diff * diff;
    }
    hrv->rmssd_ms = sqrtf(succ_diff_sq_sum / (float)(hrv->count - 1));

    // Stress Index calculation: Inverse relationship with RMSSD & SDNN
    // Lower RMSSD indicates higher sympathetic stress (Normal RMSSD: 20-50ms)
    float stress = 100.0f - (hrv->rmssd_ms * 2.0f + hrv->sdnn_ms * 1.0f);
    if (stress < 0.0f) stress = 0.0f;
    if (stress > 100.0f) stress = 100.0f;

    hrv->stress_index = stress;
    if (stress > 80.0f || hrv->rmssd_ms < 10.0f) {
        hrv->high_stress_alert = true;
    } else {
        hrv->high_stress_alert = false;
    }
}
