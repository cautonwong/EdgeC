#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "emg_dsp.h"

int main(void) {
    printf("[TEST] Testing EMG DSP Feature Extractor...\n");

    emg_dsp_analyzer_t analyzer;
    emg_dsp_init(&analyzer);

    // Generate synthetic 60 Hz EMG sine wave at 1000 Hz sample rate
    float sample_rate = 1000.0f;
    for (int i = 0; i < EMG_FFT_SIZE; i++) {
        float t = (float)i / sample_rate;
        float sample = 1.5f * sinf(2.0f * (float)M_PI * 60.0f * t);
        emg_dsp_push_sample(&analyzer, sample);
    }

    emg_dsp_calculate_features(&analyzer, sample_rate);

    printf("EMG Total Power: %.2f, MPF: %.2f Hz, MF: %.2f Hz, Fatigue: %s\n", analyzer.total_power,
           analyzer.mean_frequency, analyzer.median_frequency,
           analyzer.fatigue_flag ? "YES" : "NO");

    assert(analyzer.total_power > 0.0f);
    // Peak frequency should be near 60 Hz
    assert(analyzer.mean_frequency > 40.0f && analyzer.mean_frequency < 80.0f);

    printf("[TEST] EMG DSP Feature Extractor PASSED!\n");
    return 0;
}
