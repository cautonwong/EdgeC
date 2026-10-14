/**
 * @file ai_graph_pruner.h
 * @brief Neural Network Computation Graph Topological Pruner & Deadlock-Free Compiler (AEP-708 / Issue #241)
 *
 * Implements edge compiler optimization passes for on-device deep learning:
 * - Computation DAG representation with fixed-capacity zero-heap storage;
 * - Backward reachability dead-node elimination (eliminates dead branches without outputs);
 * - Redundant identity / dropout / reshape folding;
 * - Operator fusion identification (Conv+BatchNorm, Conv+ReLU, Gemm+Bias);
 * - Kahn's topological sort & cycle/deadlock detection;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_GRAPH_PRUNER_H
#define EDGE_AI_GRAPH_PRUNER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Constants & Enumerations                                                   */
/* -------------------------------------------------------------------------- */

#define AI_GRAPH_MAX_NODES            64
#define AI_GRAPH_MAX_EDGES_PER_NODE   8
#define AI_GRAPH_MAX_NAME_LEN         32

typedef enum {
    AI_OP_INPUT        = 0,
    AI_OP_OUTPUT       = 1,
    AI_OP_IDENTITY     = 2,
    AI_OP_DROPOUT      = 3,
    AI_OP_RESHAPE      = 4,
    AI_OP_CONV1D       = 5,
    AI_OP_CONV2D       = 6,
    AI_OP_BATCHNORM    = 7,
    AI_OP_RELU         = 8,
    AI_OP_GELU         = 9,
    AI_OP_MATMUL       = 10,
    AI_OP_ADD          = 11,
    AI_OP_DEAD_BRANCH  = 12
} ai_op_type_t;

typedef enum {
    AI_NODE_STATUS_ACTIVE    = 0,
    AI_NODE_STATUS_PRUNED    = 1,
    AI_NODE_STATUS_FOLDED    = 2,
    AI_NODE_STATUS_FUSED     = 3
} ai_node_status_t;

typedef struct {
    uint32_t id;
    char name[AI_GRAPH_MAX_NAME_LEN];
    ai_op_type_t op_type;
    ai_node_status_t status;
    uint32_t num_inputs;
    uint32_t input_ids[AI_GRAPH_MAX_EDGES_PER_NODE];
    uint32_t num_outputs;
    uint32_t output_ids[AI_GRAPH_MAX_EDGES_PER_NODE];
    int32_t fused_with_id;
} ai_graph_node_t;

typedef struct {
    uint32_t total_nodes;
    uint32_t active_nodes;
    uint32_t pruned_dead_nodes;
    uint32_t folded_identity_nodes;
    uint32_t fused_operator_pairs;
    bool is_acyclic_dag;
    float graph_size_reduction_pct;
} ai_graph_stats_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_GRAPH_PRUNER_STORAGE_SIZE  32768

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_GRAPH_PRUNER_STORAGE_SIZE];
} ai_graph_pruner_storage_t;

typedef struct ai_graph_pruner ai_graph_pruner_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Graph Pruner in caller-allocated buffer
 */
ai_graph_pruner_t *ai_graph_pruner_create_in(ai_graph_pruner_storage_t *storage);

/**
 * @brief Add a node / operator to the graph
 */
int32_t ai_graph_pruner_add_node(ai_graph_pruner_t *pruner,
                                 const char *name,
                                 ai_op_type_t op_type);

/**
 * @brief Add a directed dependency edge (src -> dst)
 */
bool ai_graph_pruner_add_edge(ai_graph_pruner_t *pruner, uint32_t src_id, uint32_t dst_id);

/**
 * @brief Run full optimization passes (dead-node pruning, identity folding, fusion, topological sort)
 */
bool ai_graph_pruner_optimize(ai_graph_pruner_t *pruner);

/**
 * @brief Get node by ID
 */
const ai_graph_node_t *ai_graph_pruner_get_node(const ai_graph_pruner_t *pruner, uint32_t node_id);

/**
 * @brief Query compilation and pruning statistics
 */
bool ai_graph_pruner_get_stats(const ai_graph_pruner_t *pruner, ai_graph_stats_t *stats);

#include "edge_algo_port.h"

/**
 * @brief Emit pruning telemetry via Consumer-Defined Algo Sink
 */
int ai_graph_pruner_emit_metrics(const ai_graph_pruner_t *pruner, const edge_algo_sink_t *sink);

/**
 * @brief Synchronize pruning telemetry to DataHub (Adapter bridge)
 */
int ai_graph_pruner_sync_datahub(const ai_graph_pruner_t *pruner, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_GRAPH_PRUNER_H */
