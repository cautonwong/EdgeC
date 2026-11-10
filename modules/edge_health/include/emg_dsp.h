#ifndef EMG_DSP_H
#define EMG_DSP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EMG_FFT_SIZE 128

typedef struct {
    float raw_samples[EMG_FFT_SIZE];
    size_t sample_count;
    float mean_frequency;     // MPF (Mean Power Frequency) in Hz
    float median_frequency;   // MF (Median Power Frequency) in Hz
    float total_power;        // Total EMG signal energy
    bool fatigue_flag;        // True if MPF drops below threshold (muscle fatigue)
} emg_dsp_analyzer_t;

void emg_dsp_init(emg_dsp_analyzer_t *analyzer);

int emg_dsp_push_sample(emg_dsp_analyzer_t *analyzer, float sample);

void emg_dsp_calculate_features(emg_dsp_analyzer_t *analyzer, float sample_rate_hz);

#ifdef __cplusplus
}
#endif

#endif // EMG_DSP_H
