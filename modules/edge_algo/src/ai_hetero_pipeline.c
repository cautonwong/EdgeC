/**
 * @file ai_hetero_pipeline.c
 * @brief Heterogeneous Multi-Backend Pipeline Orchestrator Implementation (AEP-710 / Issue #243)
 */

#include "ai_hetero_pipeline.h"
#include <string.h>

struct ai_hetero_pipeline {
    uint32_t num_subgraphs;
    ai_hetero_subgraph_t subgraphs[AI_HETERO_MAX_SUBGRAPHS];
    ai_hetero_pipeline_metrics_t metrics;
};

_Static_assert(sizeof(struct ai_hetero_pipeline) <= sizeof(ai_hetero_pipeline_storage_t),
               "ai_hetero_pipeline_storage_t size is smaller than internal struct ai_hetero_pipeline");

ai_hetero_pipeline_t *ai_hetero_pipeline_create_in(ai_hetero_pipeline_storage_t *storage) {
    if (!storage) return NULL;

    struct ai_hetero_pipeline *pipe = (struct ai_hetero_pipeline *)storage->raw;
    memset(pipe, 0, sizeof(*pipe));

    return pipe;
}

int ai_hetero_pipeline_partition(ai_hetero_pipeline_t *pipe,
                                 const ai_hetero_op_spec_t *ops,
                                 uint32_t num_ops) {
    if (!pipe || !ops || num_ops == 0) return -1;

    memset(pipe->subgraphs, 0, sizeof(pipe->subgraphs));
    pipe->num_subgraphs = 0;

    ai_hetero_subgraph_t *curr = NULL;

    for (uint32_t i = 0; i < num_ops; ++i) {
        if (!curr || curr->target_backend != ops[i].preferred_backend ||
            curr->num_ops >= AI_HETERO_MAX_OPS_PER_STAGE) {
            if (pipe->num_subgraphs >= AI_HETERO_MAX_SUBGRAPHS) {
                return -1; /* Exceeded maximum subgraphs */
            }
            curr = &pipe->subgraphs[pipe->num_subgraphs];
            curr->subgraph_id = pipe->num_subgraphs++;
            curr->target_backend = ops[i].preferred_backend;
            curr->num_ops = 0;
            curr->total_stage_cost_us = 0;
        }

        curr->op_ids[curr->num_ops++] = ops[i].op_id;
        curr->total_stage_cost_us += ops[i].estimated_cost_us;
    }

    /* Compute pipeline and sequential timings */
    uint32_t seq_time = 0;
    uint32_t max_stage = 0;
    for (uint32_t s = 0; s < pipe->num_subgraphs; ++s) {
        seq_time += pipe->subgraphs[s].total_stage_cost_us;
        if (pipe->subgraphs[s].total_stage_cost_us > max_stage) {
            max_stage = pipe->subgraphs[s].total_stage_cost_us;
        }
    }

    uint32_t dma_transfers = (pipe->num_subgraphs > 1) ? (pipe->num_subgraphs - 1) : 0;
    uint32_t dma_overhead = dma_transfers * 10; /* 10 us per DMA boundary handshake */
    uint32_t pipe_time = max_stage + dma_overhead;

    pipe->metrics.num_subgraphs = pipe->num_subgraphs;
    pipe->metrics.sequential_time_us = seq_time;
    pipe->metrics.pipelined_time_us = pipe_time;
    pipe->metrics.num_dma_transfers = dma_transfers;

    if (seq_time > 0 && pipe_time < seq_time) {
        pipe->metrics.pipeline_overlap_pct = 100.0f * (1.0f - ((float)pipe_time / (float)seq_time));
    } else {
        pipe->metrics.pipeline_overlap_pct = 0.0f;
    }

    return 0;
}

int ai_hetero_pipeline_execute_step(ai_hetero_pipeline_t *pipe) {
    if (!pipe || pipe->num_subgraphs == 0) return -1;
    /* Simulated pipelined step execution */
    return 0;
}

bool ai_hetero_pipeline_get_metrics(const ai_hetero_pipeline_t *pipe,
                                    ai_hetero_pipeline_metrics_t *metrics) {
    if (!pipe || !metrics) return false;
    *metrics = pipe->metrics;
    return true;
}

int ai_hetero_pipeline_sync_datahub(const ai_hetero_pipeline_t *pipe, datahub_t *hub) {
    if (!pipe || !hub) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 262178, (double)pipe->metrics.num_subgraphs, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, 262179, (double)pipe->metrics.pipeline_overlap_pct, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}
