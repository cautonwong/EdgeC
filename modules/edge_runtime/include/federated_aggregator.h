/**
 * @file federated_aggregator.h
 * @brief Edge Federated Learning Model Weight Delta Aggregator (FedAvg)
 */

#ifndef FEDERATED_AGGREGATOR_H
#define FEDERATED_AGGREGATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t round_id;
    size_t num_weights;
    float *global_weights;
    size_t num_clients;
    uint32_t total_samples;
} fed_aggregator_t;

/**
 * @brief Initializes federated aggregator instance
 * @param agg Aggregator handle
 * @param num_weights Number of model weights
 * @param initial_weights Initial global weights (copied)
 * @return 0 on success, negative error code on failure
 */
int fed_aggregator_init(fed_aggregator_t *agg, size_t num_weights, const float *initial_weights);

/**
 * @brief Cleans up federated aggregator resources
 * @param agg Aggregator handle
 */
void fed_aggregator_free(fed_aggregator_t *agg);

/**
 * @brief Accumulates local client weight delta (FedAvg weight algorithm)
 * @param agg Aggregator handle
 * @param client_weights Client model weights
 * @param num_samples Number of local dataset samples
 * @param accum_buf Accumulation buffer array of floats (size num_weights)
 * @return 0 on success, negative error code on failure
 */
int fed_aggregator_accumulate(fed_aggregator_t *agg, const float *client_weights,
                              uint32_t num_samples, float *accum_buf);

/**
 * @brief Finalizes global weight update after receiving client updates
 * @param agg Aggregator handle
 * @param accum_buf Accumulation buffer array of floats
 * @return 0 on success, negative error code on failure
 */
int fed_aggregator_finalize(fed_aggregator_t *agg, const float *accum_buf);

#ifdef __cplusplus
}
#endif

#endif /* FEDERATED_AGGREGATOR_H */
