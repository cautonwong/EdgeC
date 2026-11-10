#ifndef APNEA_DETECTOR_H
#define APNEA_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float respiration_rate_brpm; // Breaths per minute (e.g. 12..20 brpm)
    float apnea_duration_sec;    // Duration of cessation of airflow/chest motion
    uint32_t apnea_events_count; // Total apnea events (>10s) in session
    float ahi_index;             // Apnea-Hypopnea Index (events / hour)
    bool obstructive_apnea_alert;
} apnea_detector_t;

void apnea_detector_init(apnea_detector_t *det);

void apnea_detector_update(apnea_detector_t *det, float chest_accel_norm, float audio_db, float dt_sec, float total_session_hours);

#ifdef __cplusplus
}
#endif

#endif // APNEA_DETECTOR_H
