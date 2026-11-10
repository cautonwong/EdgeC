#ifndef TREMOR_ANALYZER_H
#define TREMOR_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float peak_frequency_hz; // Tremor frequency (3..12 Hz)
    float tremor_power;      // Energy in 3-12Hz band
    bool resting_tremor_flag; // True if peak frequency in 4-6 Hz range
    bool action_tremor_flag;  // True if peak frequency in 8-12 Hz range
} tremor_analysis_result_t;

void tremor_analyzer_init(tremor_analysis_result_t *res);

void tremor_analyzer_compute(tremor_analysis_result_t *res, const float *accel_data, size_t sample_count, float sample_rate_hz);

#ifdef __cplusplus
}
#endif

#endif // TREMOR_ANALYZER_H
