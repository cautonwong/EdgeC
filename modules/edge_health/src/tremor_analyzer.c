#include "tremor_analyzer.h"
#include <math.h>
#include <string.h>

void tremor_analyzer_init(tremor_analysis_result_t *res) {
    if (!res) return;
    memset(res, 0, sizeof(tremor_analysis_result_t));
}

void tremor_analyzer_compute(tremor_analysis_result_t *res, const float *accel_data, size_t sample_count, float sample_rate_hz) {
    if (!res || !accel_data || sample_count == 0 || sample_rate_hz <= 0.0f) return;

    float max_power = 0.0f;
    float peak_freq = 0.0f;
    float total_band_power = 0.0f;

    float bin_width = sample_rate_hz / (float)sample_count;

    // Scan frequency spectrum between 3.0 Hz and 12.0 Hz
    for (size_t k = 1; k < sample_count / 2; k++) {
        float freq = (float)k * bin_width;
        if (freq < 3.0f || freq > 12.0f) continue;

        float real_sum = 0.0f;
        float imag_sum = 0.0f;

        for (size_t n = 0; n < sample_count; n++) {
            float angle = 2.0f * (float)M_PI * (float)k * (float)n / (float)sample_count;
            real_sum += accel_data[n] * cosf(angle);
            imag_sum -= accel_data[n] * sinf(angle);
        }

        float mag_sq = (real_sum * real_sum + imag_sum * imag_sum) / (float)sample_count;
        total_band_power += mag_sq;

        if (mag_sq > max_power) {
            max_power = mag_sq;
            peak_freq = freq;
        }
    }

    res->peak_frequency_hz = peak_freq;
    res->tremor_power = total_band_power;

    // Resting Tremor (Parkinsonian): 4 - 6 Hz
    if (peak_freq >= 4.0f && peak_freq <= 6.2f && max_power > 0.5f) {
        res->resting_tremor_flag = true;
    } else {
        res->resting_tremor_flag = false;
    }

    // Action/Essential Tremor: 8 - 12 Hz
    if (peak_freq >= 7.8f && peak_freq <= 12.0f && max_power > 0.5f) {
        res->action_tremor_flag = true;
    } else {
        res->action_tremor_flag = false;
    }
}
