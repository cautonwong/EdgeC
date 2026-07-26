#include "apnea_detector.h"
#include <string.h>

void apnea_detector_init(apnea_detector_t *det) {
    if (!det) return;
    memset(det, 0, sizeof(apnea_detector_t));
    det->respiration_rate_brpm = 16.0f; // Default baseline 16 brpm
}

void apnea_detector_update(apnea_detector_t *det, float chest_accel_norm, float audio_db, float dt_sec, float total_session_hours) {
    if (!det || dt_sec <= 0.0f) return;

    // Airflow & chest motion threshold (absence of breathing < 0.05 m/s^2 & < 30 dB)
    if (chest_accel_norm < 0.05f && audio_db < 30.0f) {
        det->apnea_duration_sec += dt_sec;

        // If respiration stops for > 10 seconds, count as an Apnea event
        if (det->apnea_duration_sec >= 10.0f && !det->obstructive_apnea_alert) {
            det->apnea_events_count++;
            det->obstructive_apnea_alert = true;
        }
    } else {
        // Respiration resumed
        det->apnea_duration_sec = 0.0f;
        det->obstructive_apnea_alert = false;
    }

    // Calculate AHI (events per hour)
    if (total_session_hours > 0.01f) {
        det->ahi_index = (float)det->apnea_events_count / total_session_hours;
    }
}
