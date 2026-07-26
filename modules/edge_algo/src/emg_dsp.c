#include "emg_dsp.h"
#include <math.h>
#include <string.h>

void emg_dsp_init(emg_dsp_analyzer_t *analyzer) {
    if (!analyzer) return;
    memset(analyzer, 0, sizeof(emg_dsp_analyzer_t));
}

int emg_dsp_push_sample(emg_dsp_analyzer_t *analyzer, float sample) {
    if (!analyzer) return -1;
    if (analyzer->sample_count >= EMG_FFT_SIZE) {
        // Shift window
        memmove(&analyzer->raw_samples[0], &analyzer->raw_samples[1], (EMG_FFT_SIZE - 1) * sizeof(float));
        analyzer->raw_samples[EMG_FFT_SIZE - 1] = sample;
    } else {
        analyzer->raw_samples[analyzer->sample_count++] = sample;
    }
    return 0;
}

void emg_dsp_calculate_features(emg_dsp_analyzer_t *analyzer, float sample_rate_hz) {
    if (!analyzer || analyzer->sample_count < EMG_FFT_SIZE || sample_rate_hz <= 0.0f) {
        return;
    }

    // Discrete Fourier Transform (DFT) power spectrum estimation for 128 points
    float power_spectrum[EMG_FFT_SIZE / 2];
    float total_pwr = 0.0f;
    float freq_sum = 0.0f;

    float bin_width = sample_rate_hz / (float)EMG_FFT_SIZE;

    for (int k = 0; k < EMG_FFT_SIZE / 2; k++) {
        float real_sum = 0.0f;
        float imag_sum = 0.0f;

        for (int n = 0; n < EMG_FFT_SIZE; n++) {
            float angle = 2.0f * (float)M_PI * (float)k * (float)n / (float)EMG_FFT_SIZE;
            real_sum += analyzer->raw_samples[n] * cosf(angle);
            imag_sum -= analyzer->raw_samples[n] * sinf(angle);
        }

        float mag_sq = (real_sum * real_sum + imag_sum * imag_sum) / (float)EMG_FFT_SIZE;
        power_spectrum[k] = mag_sq;
        total_pwr += mag_sq;

        float freq = (float)k * bin_width;
        freq_sum += freq * mag_sq;
    }

    analyzer->total_power = total_pwr;
    if (total_pwr > 1e-6f) {
        analyzer->mean_frequency = freq_sum / total_pwr;
    } else {
        analyzer->mean_frequency = 0.0f;
    }

    // Median frequency (MF) calculation
    float cum_pwr = 0.0f;
    float half_pwr = total_pwr * 0.5f;
    analyzer->median_frequency = 0.0f;

    for (int k = 0; k < EMG_FFT_SIZE / 2; k++) {
        cum_pwr += power_spectrum[k];
        if (cum_pwr >= half_pwr) {
            analyzer->median_frequency = (float)k * bin_width;
            break;
        }
    }

    // Fatigue threshold: MPF dropping under 45 Hz indicates muscle fatigue onset
    if (analyzer->mean_frequency > 0.0f && analyzer->mean_frequency < 45.0f) {
        analyzer->fatigue_flag = true;
    } else {
        analyzer->fatigue_flag = false;
    }
}
