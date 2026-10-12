/**
 * @file ai_hetero_pipeline.h
 * @brief Heterogeneous Multi-Backend (NPU/GPU/CPU/DSP) Pipeline Orchestrator (AEP-710 / Issue #243)
 *
 * Implements industrial edge heterogeneous AI subgraph partitioning & concurrent pipeline execution:
 * - Hardware target affinity matching (NPU matrix acceleration, DSP signal filtering, CPU control logic);
 * - Subgraph graph cutting & DMA boundary staging;
 * - Async ping-pong double-buffering pipeline between NPU execution and CPU/DSP preprocessing;
 * - Real-time pipeline latency overlap & hardware concurrency metrics;
 * - Zero-heap caller-allocated storage;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_HETERO_PIPELINE_H
#define EDGE_AI_HETERO_PIPELINE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Types                                                          */
/* -------------------------------------------------------------------------- */

#define AI_HETERO_MAX_SUBGRAPHS        8
#define AI_HETERO_MAX_OPS_PER_STAGE    16

typedef enum {
    AI_BACKEND_CPU = 0,
    AI_BACKEND_NPU = 1,
    AI_BACKEND_GPU = 2,
    AI_BACKEND_DSP = 3
} ai_backend_type_t;

typedef struct {
    uint32_t op_id;
    ai_backend_type_t preferred_backend;
    uint32_t estimated_cost_us;
} ai_hetero_op_spec_t;

typedef struct {
    uint32_t subgraph_id;
    ai_backend_type_t target_backend;
    uint32_t num_ops;
    uint32_t op_ids[AI_HETERO_MAX_OPS_PER_STAGE];
    uint32_t total_stage_cost_us;
} ai_hetero_subgraph_t;

typedef struct {
    uint32_t num_subgraphs;
    uint32_t sequential_time_us;
    uint32_t pipelined_time_us;
    float pipeline_overlap_pct;
    uint32_t num_dma_transfers;
} ai_hetero_pipeline_metrics_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_HETERO_PIPELINE_STORAGE_SIZE 16384

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_HETERO_PIPELINE_STORAGE_SIZE];
} ai_hetero_pipeline_storage_t;

typedef struct ai_hetero_pipeline ai_hetero_pipeline_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Heterogeneous Pipeline Orchestrator in caller-allocated buffer
 */
ai_hetero_pipeline_t *ai_hetero_pipeline_create_in(ai_hetero_pipeline_storage_t *storage);

/**
 * @brief Partition a linear graph sequence of operators into backend subgraphs
 */
int ai_hetero_pipeline_partition(ai_hetero_pipeline_t *pipe,
                                 const ai_hetero_op_spec_t *ops,
                                 uint32_t num_ops);

/**
 * @brief Execute a pipelined step (simulating concurrent overlapping execution across backends)
 */
int ai_hetero_pipeline_execute_step(ai_hetero_pipeline_t *pipe);

/**
 * @brief Query current pipeline partition metrics
 */
bool ai_hetero_pipeline_get_metrics(const ai_hetero_pipeline_t *pipe,
                                    ai_hetero_pipeline_metrics_t *metrics);

/**
 * @brief Synchronize metrics to DataHub
 */
int ai_hetero_pipeline_sync_datahub(const ai_hetero_pipeline_t *pipe, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_HETERO_PIPELINE_H */
