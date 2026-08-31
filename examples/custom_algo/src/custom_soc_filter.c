/**
 * @file custom_soc_filter.c
 * @brief Secondary Developer Example: Custom Battery SOC Estimation & Smoothing Algorithm Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "custom_soc_filter.h"

int custom_soc_filter_init(custom_soc_filter_t *filter, float alpha, float initial_soc) {
    if (!filter) return -1;
    if (alpha <= 0.0f || alpha > 1.0f) {
        alpha = 0.2f; // Default smoothing factor
    }
    filter->alpha = alpha;
    filter->filtered_soc = initial_soc;
    filter->initialized = true;
    return 0;
}

float custom_soc_filter_step(custom_soc_filter_t *filter, float raw_soc) {
    if (!filter || !filter->initialized) return raw_soc;

    // Exponential moving average filter: S_t = alpha * Y_t + (1 - alpha) * S_{t-1}
    filter->filtered_soc = filter->alpha * raw_soc + (1.0f - filter->alpha) * filter->filtered_soc;

    // Clamping to physical envelope 0.0% - 100.0%
    if (filter->filtered_soc < 0.0f) filter->filtered_soc = 0.0f;
    if (filter->filtered_soc > 100.0f) filter->filtered_soc = 100.0f;

    return filter->filtered_soc;
}

int custom_soc_filter_process_datahub(custom_soc_filter_t *filter, edgeos_datahub_t *hub, uint32_t raw_tag_id, uint32_t out_tag_id) {
    if (!filter || !hub) return -1;

    edgeos_pipe_msg_t raw_msg;
    int rc = datahub_read_shadow(hub, raw_tag_id, &raw_msg);
    if (rc != 0) {
        return -1; // Tag not yet available
    }

    float raw_val = (float)raw_msg.payload.val.f;
    float filtered_val = custom_soc_filter_step(filter, raw_val);

    edgeos_pipe_msg_t out_msg;
    pipe_msg_init_scalar(&out_msg, out_tag_id, (double)filtered_val, raw_msg.ts);
    return datahub_write_shadow(hub, &out_msg);
}
