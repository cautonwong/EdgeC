#ifndef ECG_ARRHYTHMIA_H
#define ECG_ARRHYTHMIA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float heart_rate_bpm;
    float qrs_duration_ms; // Normal: 80..120 ms
    float qt_interval_ms;  // Normal: 350..450 ms
    bool pvc_detected;     // Premature Ventricular Contraction
    bool bradycardia_alert; // HR < 50 bpm
    bool tachycardia_alert; // HR > 100 bpm
} ecg_arrhythmia_result_t;

void ecg_arrhythmia_init(ecg_arrhythmia_result_t *res);

void ecg_arrhythmia_analyze(ecg_arrhythmia_result_t *res, float rr_interval_ms, float qrs_ms, float qt_ms);

#ifdef __cplusplus
}
#endif

#endif // ECG_ARRHYTHMIA_H
