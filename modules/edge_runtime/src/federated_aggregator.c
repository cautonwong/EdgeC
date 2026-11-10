/**
 * @file federated_aggregator.c
 * @brief Edge Federated Learning Model Weight Delta Aggregator Implementation
 */

#include "federated_aggregator.h"
#include <stdlib.h>
#include <string.h>

int fed_aggregator_init(fed_aggregator_t *agg, size_t num_weights, const float *initial_weights) {
    if (!agg || num_weights == 0) {
        return -1;
    }

    memset(agg, 0, sizeof(fed_aggregator_t));
    agg->num_weights = num_weights;
    agg->global_weights = (float *)calloc(num_weights, sizeof(float));
    if (!agg->global_weights) {
        return -2;
    }

    if (initial_weights) {
        memcpy(agg->global_weights, initial_weights, num_weights * sizeof(float));
    }

    agg->round_id = 1;
    return 0;
}

void fed_aggregator_free(fed_aggregator_t *agg) {
    if (agg) {
        if (agg->global_weights) {
            free(agg->global_weights);
            agg->global_weights = NULL;
        }
        memset(agg, 0, sizeof(fed_aggregator_t));
    }
}

int fed_aggregator_accumulate(fed_aggregator_t *agg, const float *client_weights,
                              uint32_t num_samples, float *accum_buf) {
    if (!agg || !client_weights || !accum_buf || num_samples == 0) {
        return -1;
    }

    for (size_t i = 0; i < agg->num_weights; i++) {
        accum_buf[i] += client_weights[i] * (float)num_samples;
    }

    agg->num_clients++;
    agg->total_samples += num_samples;
    return 0;
}

int fed_aggregator_finalize(fed_aggregator_t *agg, const float *accum_buf) {
    if (!agg || !accum_buf || agg->total_samples == 0) {
        return -1;
    }

    float inv_samples = 1.0f / (float)agg->total_samples;
    for (size_t i = 0; i < agg->num_weights; i++) {
        agg->global_weights[i] = accum_buf[i] * inv_samples;
    }

    agg->round_id++;
    agg->num_clients = 0;
    agg->total_samples = 0;
    return 0;
}
