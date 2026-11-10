/**
 * @file ai_graph_pruner.c
 * @brief Neural Network Computation Graph Topological Pruner Implementation (AEP-708 / Issue #241)
 */

#include "ai_graph_pruner.h"
#include <string.h>

struct ai_graph_pruner {
    uint32_t num_nodes;
    ai_graph_node_t nodes[AI_GRAPH_MAX_NODES];

    uint32_t pruned_dead_count;
    uint32_t folded_identity_count;
    uint32_t fused_pair_count;
    bool is_acyclic;

    uint32_t topo_order[AI_GRAPH_MAX_NODES];
    uint32_t topo_count;
};

_Static_assert(sizeof(struct ai_graph_pruner) <= sizeof(ai_graph_pruner_storage_t),
               "ai_graph_pruner_storage_t size is smaller than internal struct ai_graph_pruner");

ai_graph_pruner_t *ai_graph_pruner_create_in(ai_graph_pruner_storage_t *storage) {
    if (!storage) return NULL;

    struct ai_graph_pruner *pruner = (struct ai_graph_pruner *)storage->raw;
    memset(pruner, 0, sizeof(*pruner));
    pruner->is_acyclic = true;

    return pruner;
}

int32_t ai_graph_pruner_add_node(ai_graph_pruner_t *pruner,
                                 const char *name,
                                 ai_op_type_t op_type) {
    if (!pruner || pruner->num_nodes >= AI_GRAPH_MAX_NODES) {
        return -1;
    }

    uint32_t id = pruner->num_nodes++;
    ai_graph_node_t *node = &pruner->nodes[id];
    memset(node, 0, sizeof(*node));
    node->id = id;
    node->op_type = op_type;
    node->status = AI_NODE_STATUS_ACTIVE;
    node->fused_with_id = -1;

    if (name) {
        strncpy(node->name, name, AI_GRAPH_MAX_NAME_LEN - 1);
        node->name[AI_GRAPH_MAX_NAME_LEN - 1] = '\0';
    }

    return (int32_t)id;
}

bool ai_graph_pruner_add_edge(ai_graph_pruner_t *pruner, uint32_t src_id, uint32_t dst_id) {
    if (!pruner || src_id >= pruner->num_nodes || dst_id >= pruner->num_nodes || src_id == dst_id) {
        return false;
    }

    ai_graph_node_t *src = &pruner->nodes[src_id];
    ai_graph_node_t *dst = &pruner->nodes[dst_id];

    if (src->num_outputs >= AI_GRAPH_MAX_EDGES_PER_NODE ||
        dst->num_inputs >= AI_GRAPH_MAX_EDGES_PER_NODE) {
        return false;
    }

    /* Check duplicate */
    for (uint32_t i = 0; i < src->num_outputs; ++i) {
        if (src->output_ids[i] == dst_id) return true;
    }

    src->output_ids[src->num_outputs++] = dst_id;
    dst->input_ids[dst->num_inputs++] = src_id;

    return true;
}

static void pass_dead_node_elimination(struct ai_graph_pruner *pruner) {
    bool reachable[AI_GRAPH_MAX_NODES] = {false};
    uint32_t queue[AI_GRAPH_MAX_NODES];
    uint32_t head = 0, tail = 0;

    /* Start from all OUTPUT nodes */
    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        if (pruner->nodes[i].op_type == AI_OP_OUTPUT) {
            reachable[i] = true;
            queue[tail++] = i;
        }
    }

    /* Backward BFS */
    while (head < tail) {
        uint32_t curr_id = queue[head++];
        const ai_graph_node_t *curr = &pruner->nodes[curr_id];

        for (uint32_t in_idx = 0; in_idx < curr->num_inputs; ++in_idx) {
            uint32_t parent_id = curr->input_ids[in_idx];
            if (parent_id < pruner->num_nodes && !reachable[parent_id]) {
                reachable[parent_id] = true;
                queue[tail++] = parent_id;
            }
        }
    }

    /* Mark unreachables as PRUNED */
    pruner->pruned_dead_count = 0;
    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        if (!reachable[i] && pruner->nodes[i].status == AI_NODE_STATUS_ACTIVE) {
            pruner->nodes[i].status = AI_NODE_STATUS_PRUNED;
            pruner->pruned_dead_count++;
        }
    }
}

static void pass_identity_folding(struct ai_graph_pruner *pruner) {
    pruner->folded_identity_count = 0;

    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        ai_graph_node_t *node = &pruner->nodes[i];
        if (node->status != AI_NODE_STATUS_ACTIVE) continue;

        if ((node->op_type == AI_OP_IDENTITY || node->op_type == AI_OP_DROPOUT) &&
            node->num_inputs == 1) {
            uint32_t upstream_id = node->input_ids[0];
            ai_graph_node_t *upstream = &pruner->nodes[upstream_id];

            /* Redirect all consumers of node to upstream */
            for (uint32_t o = 0; o < node->num_outputs; ++o) {
                uint32_t downstream_id = node->output_ids[o];
                ai_graph_node_t *downstream = &pruner->nodes[downstream_id];

                for (uint32_t d_in = 0; d_in < downstream->num_inputs; ++d_in) {
                    if (downstream->input_ids[d_in] == node->id) {
                        downstream->input_ids[d_in] = upstream_id;
                    }
                }
                /* Add downstream to upstream output if not already present */
                bool found = false;
                for (uint32_t u_out = 0; u_out < upstream->num_outputs; ++u_out) {
                    if (upstream->output_ids[u_out] == downstream_id) {
                        found = true;
                        break;
                    }
                }
                if (!found && upstream->num_outputs < AI_GRAPH_MAX_EDGES_PER_NODE) {
                    upstream->output_ids[upstream->num_outputs++] = downstream_id;
                }
            }

            node->status = AI_NODE_STATUS_FOLDED;
            pruner->folded_identity_count++;
        }
    }
}

static void pass_operator_fusion(struct ai_graph_pruner *pruner) {
    pruner->fused_pair_count = 0;

    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        ai_graph_node_t *node = &pruner->nodes[i];
        if (node->status != AI_NODE_STATUS_ACTIVE) continue;

        /* Conv1D/2D + ReLU or Conv + BatchNorm fusion */
        if ((node->op_type == AI_OP_CONV1D || node->op_type == AI_OP_CONV2D || node->op_type == AI_OP_MATMUL) &&
            node->num_outputs == 1) {
            uint32_t next_id = node->output_ids[0];
            ai_graph_node_t *next_node = &pruner->nodes[next_id];

            if (next_node->status == AI_NODE_STATUS_ACTIVE &&
                (next_node->op_type == AI_OP_RELU || next_node->op_type == AI_OP_BATCHNORM || next_node->op_type == AI_OP_GELU) &&
                next_node->num_inputs == 1) {
                next_node->status = AI_NODE_STATUS_FUSED;
                next_node->fused_with_id = (int32_t)node->id;
                pruner->fused_pair_count++;
            }
        }
    }
}

static void pass_topological_sort(struct ai_graph_pruner *pruner) {
    uint32_t in_degree[AI_GRAPH_MAX_NODES] = {0};
    uint32_t active_count = 0;

    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        if (pruner->nodes[i].status == AI_NODE_STATUS_ACTIVE ||
            pruner->nodes[i].status == AI_NODE_STATUS_FUSED) {
            active_count++;
            const ai_graph_node_t *node = &pruner->nodes[i];
            for (uint32_t o = 0; o < node->num_outputs; ++o) {
                uint32_t dst_id = node->output_ids[o];
                if (pruner->nodes[dst_id].status == AI_NODE_STATUS_ACTIVE ||
                    pruner->nodes[dst_id].status == AI_NODE_STATUS_FUSED) {
                    in_degree[dst_id]++;
                }
            }
        }
    }

    uint32_t queue[AI_GRAPH_MAX_NODES];
    uint32_t head = 0, tail = 0;

    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        if ((pruner->nodes[i].status == AI_NODE_STATUS_ACTIVE ||
             pruner->nodes[i].status == AI_NODE_STATUS_FUSED) &&
            in_degree[i] == 0) {
            queue[tail++] = i;
        }
    }

    pruner->topo_count = 0;
    while (head < tail) {
        uint32_t u = queue[head++];
        pruner->topo_order[pruner->topo_count++] = u;

        const ai_graph_node_t *node = &pruner->nodes[u];
        for (uint32_t o = 0; o < node->num_outputs; ++o) {
            uint32_t dst_id = node->output_ids[o];
            if (pruner->nodes[dst_id].status == AI_NODE_STATUS_ACTIVE ||
                pruner->nodes[dst_id].status == AI_NODE_STATUS_FUSED) {
                if (--in_degree[dst_id] == 0) {
                    queue[tail++] = dst_id;
                }
            }
        }
    }

    pruner->is_acyclic = (pruner->topo_count == active_count);
}

bool ai_graph_pruner_optimize(ai_graph_pruner_t *pruner) {
    if (!pruner) return false;

    pass_dead_node_elimination(pruner);
    pass_identity_folding(pruner);
    pass_operator_fusion(pruner);
    pass_topological_sort(pruner);

    return pruner->is_acyclic;
}

const ai_graph_node_t *ai_graph_pruner_get_node(const ai_graph_pruner_t *pruner, uint32_t node_id) {
    if (!pruner || node_id >= pruner->num_nodes) return NULL;
    return &pruner->nodes[node_id];
}

bool ai_graph_pruner_get_stats(const ai_graph_pruner_t *pruner, ai_graph_stats_t *stats) {
    if (!pruner || !stats) return false;

    stats->total_nodes = pruner->num_nodes;
    stats->pruned_dead_nodes = pruner->pruned_dead_count;
    stats->folded_identity_nodes = pruner->folded_identity_count;
    stats->fused_operator_pairs = pruner->fused_pair_count;
    stats->is_acyclic_dag = pruner->is_acyclic;

    uint32_t active = 0;
    for (uint32_t i = 0; i < pruner->num_nodes; ++i) {
        if (pruner->nodes[i].status == AI_NODE_STATUS_ACTIVE) {
            active++;
        }
    }
    stats->active_nodes = active;

    if (stats->total_nodes > 0) {
        uint32_t eliminated = stats->pruned_dead_nodes + stats->folded_identity_nodes + stats->fused_operator_pairs;
        stats->graph_size_reduction_pct = 100.0f * ((float)eliminated / (float)stats->total_nodes);
    } else {
        stats->graph_size_reduction_pct = 0.0f;
    }

    return true;
}

int ai_graph_pruner_emit_metrics(const ai_graph_pruner_t *pruner, const edge_algo_sink_t *sink) {
    if (!pruner || !sink) return -1;

    ai_graph_stats_t stats;
    if (!ai_graph_pruner_get_stats(pruner, &stats)) return -1;

    edge_algo_sink_emit_scalar(sink, 262172, (double)stats.pruned_dead_nodes, 0);
    edge_algo_sink_emit_scalar(sink, 262173, (double)stats.fused_operator_pairs, 0);

    return 0;
}
