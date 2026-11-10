#include "ecg_arrhythmia.h"
#include <string.h>

void ecg_arrhythmia_init(ecg_arrhythmia_result_t *res) {
    if (!res) return;
    memset(res, 0, sizeof(ecg_arrhythmia_result_t));
}

void ecg_arrhythmia_analyze(ecg_arrhythmia_result_t *res, float rr_interval_ms, float qrs_ms, float qt_ms) {
    if (!res || rr_interval_ms <= 1.0f) return;

    // Calculate Heart Rate (BPM) = 60000 / RR_ms
    float hr = 60000.0f / rr_interval_ms;
    res->heart_rate_bpm = hr;
    res->qrs_duration_ms = qrs_ms;
    res->qt_interval_ms = qt_ms;

    // Bradycardia / Tachycardia alerts
    res->bradycardia_alert = (hr < 50.0f);
    res->tachycardia_alert = (hr > 100.0f);

    // Premature Ventricular Contraction (PVC) detection criteria:
    // Wide QRS (> 120ms) coupled with shortened RR interval (< 500ms)
    if (qrs_ms > 120.0f && rr_interval_ms < 500.0f) {
        res->pvc_detected = true;
    } else {
        res->pvc_detected = false;
    }
}
