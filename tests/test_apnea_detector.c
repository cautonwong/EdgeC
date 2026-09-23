#include <assert.h>
#include <stdio.h>

#include "apnea_detector.h"

int main(void) {
    printf("[TEST] Testing Respiration Rate & Apnea Detector...\n");

    apnea_detector_t det;
    apnea_detector_init(&det);

    // Simulate normal breathing (chest_accel=0.2g, audio=50dB for 5 sec)
    for (int i = 0; i < 5; i++) {
        apnea_detector_update(&det, 0.2f, 50.0f, 1.0f, 1.0f);
    }
    assert(!det.obstructive_apnea_alert);
    assert(det.apnea_events_count == 0);

    // Simulate 12 seconds of apnea (cessation of breath: accel=0.01g, audio=20dB)
    for (int i = 0; i < 12; i++) {
        apnea_detector_update(&det, 0.01f, 20.0f, 1.0f, 1.0f);
    }

    printf("Apnea Duration: %.1f s, Events: %u, AHI: %.1f, Alert: %s\n", det.apnea_duration_sec,
           det.apnea_events_count, det.ahi_index, det.obstructive_apnea_alert ? "YES" : "NO");

    assert(det.obstructive_apnea_alert);
    assert(det.apnea_events_count == 1);

    printf("[TEST] Respiration Rate & Apnea Detector PASSED!\n");
    return 0;
}
