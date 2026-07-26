#ifndef HRV_ANALYZER_H
#define HRV_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RR_INTERVALS 64

typedef struct {
    float rr_intervals_ms[MAX_RR_INTERVALS];
    size_t count;
    float sdnn_ms;   // Standard deviation of NN intervals
    float rmssd_ms;  // Root mean square of successive differences
    float stress_index; // 0..100 Stress Index
    bool high_stress_alert;
} hrv_analyzer_t;

void hrv_analyzer_init(hrv_analyzer_t *hrv);

int hrv_analyzer_push_rr(hrv_analyzer_t *hrv, float rr_ms);

void hrv_analyzer_calculate(hrv_analyzer_t *hrv);

#ifdef __cplusplus
}
#endif

#endif // HRV_ANALYZER_H
