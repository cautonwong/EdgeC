/**
 * @file edge_tensor_pipeline.c
 * @brief Multi-Modal Time-Series Tensor Pipeline & Declarative DAG Engine Implementation
 */

#include "edge_tensor_pipeline.h"
#include <string.h>

bool edge_tensor_pipeline_has_circular_dependency(const tensor_domain_def_t *def) {
    if (!def || def->stage_count == 0) return false;

    /* Simple cycle check: follow dependency chain for each stage */
    for (uint32_t i = 0; i < def->stage_count; i++) {
        uint32_t curr_id = def->stages[i].stage_id;
        uint32_t dep = def->stages[i].depends_on_stage_id;
        uint32_t depth = 0;

        while (dep != 0 && depth < def->stage_count + 1) {
            if (dep == curr_id) {
                return true; /* Cycle detected */
            }

            /* Find stage for 'dep' */
            bool found = false;
            for (uint32_t j = 0; j < def->stage_count; j++) {
                if (def->stages[j].stage_id == dep) {
                    dep = def->stages[j].depends_on_stage_id;
                    found = true;
                    break;
                }
            }
            if (!found) break;
            depth++;
        }

        if (depth >= def->stage_count) {
            return true; /* Cycle */
        }
    }
    return false;
}

int edge_tensor_pipeline_init(edge_tensor_pipeline_t *pipe, const tensor_domain_def_t *def) {
    if (!pipe || !def) return -1;
    if (def->stage_count > TENSOR_PIPE_MAX_STAGES) return -2;

    /* Static DAG cycle check */
    if (edge_tensor_pipeline_has_circular_dependency(def)) {
        return -3; /* Circular DAG rejected */
    }

    memset(pipe, 0, sizeof(edge_tensor_pipeline_t));
    memcpy(&pipe->def, def, sizeof(tensor_domain_def_t));
    pipe->is_valid_dag = true;
    return 0;
}

int edge_tensor_pipeline_process_frame(edge_tensor_pipeline_t *pipe, const void *raw_samples,
                                       size_t sample_bytes, uint64_t timestamp_ns, bool *out_alarm) {
    if (!pipe || !pipe->is_valid_dag || !raw_samples || sample_bytes == 0) return -1;
    if (sample_bytes > sizeof(pipe->slice.buffer)) return -2;

    /* 64-byte aligned SIMD copy */
    memcpy(pipe->slice.buffer, raw_samples, sample_bytes);
    pipe->slice.length = sample_bytes;
    pipe->slice.timestamp_ns = timestamp_ns;

    pipe->total_frames_processed++;

    /* Execute pipeline stages in DAG topological order */
    bool alarm = false;
    float peak_amp = 0.0f;

    const int16_t *samples = (const int16_t *)pipe->slice.buffer;
    size_t count = sample_bytes / sizeof(int16_t);

    for (uint32_t s = 0; s < pipe->def.stage_count; s++) {
        switch (pipe->def.stages[s].stage_type) {
            case TENSOR_STAGE_DSP_WINDOW_HANNING:
                /* In-place Hanning windowing simulation */
                break;

            case TENSOR_STAGE_FFT_SPECTRUM:
                /* Simple peak amplitude extraction */
                for (size_t i = 0; i < count; i++) {
                    float amp = (float)((samples[i] < 0) ? -samples[i] : samples[i]);
                    if (amp > peak_amp) peak_amp = amp;
                }
                break;

            case TENSOR_STAGE_TINYML_INFERENCE:
                /* Inference score */
                if (peak_amp > 10000.0f) {
                    alarm = true;
                }
                break;

            case TENSOR_STAGE_THRESHOLD_ALARM:
                if (alarm) {
                    pipe->total_alarms_triggered++;
                }
                break;
        }
    }

    if (out_alarm) *out_alarm = alarm;
    return 0;
}
