#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "tremor_analyzer.h"

int main(void) {
    printf("[TEST] Testing Motion Sensor Tremor Spectral Analyzer...\n");

    tremor_analysis_result_t res;
    tremor_analyzer_init(&res);

    // Generate synthetic 5.0 Hz resting tremor signal at 100 Hz sample rate
    float accel[200];
    float sample_rate = 100.0f;
    for (int i = 0; i < 200; i++) {
        float t = (float)i / sample_rate;
        accel[i] = 2.0f * sinf(2.0f * (float)M_PI * 5.0f * t);
    }

    tremor_analyzer_compute(&res, accel, 200, sample_rate);

    printf("Peak Frequency: %.1f Hz, Power: %.2f, Resting Tremor: %s, Action Tremor: %s\n",
           res.peak_frequency_hz, res.tremor_power,
           res.resting_tremor_flag ? "YES" : "NO",
           res.action_tremor_flag ? "YES" : "NO");

    assert(res.peak_frequency_hz >= 4.5f && res.peak_frequency_hz <= 5.5f);
    assert(res.resting_tremor_flag);
    assert(!res.action_tremor_flag);

    printf("[TEST] Motion Sensor Tremor Spectral Analyzer PASSED!\n");
    return 0;
}
