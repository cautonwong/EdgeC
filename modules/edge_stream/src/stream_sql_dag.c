#include "stream_sql_dag.h"
#include <string.h>

void stream_dag_init(stream_dag_t *dag) {
    if (!dag) return;
    memset(dag, 0, sizeof(stream_dag_t));
}

int stream_dag_add_node(stream_dag_t *dag, uint32_t node_id, dag_op_type_t op, uint16_t src_tag, uint16_t dst_tag, double param, double offset_param) {
    if (!dag || dag->node_count >= DAG_MAX_NODES) return -1;

    dag_node_t *n = &dag->nodes[dag->node_count++];
    n->node_id = node_id;
    n->op = op;
    n->src_tag = src_tag;
    n->dst_tag = dst_tag;
    n->param = param;
    n->offset_param = offset_param;
    n->acc = 0.0;
    n->current_count = 0;
    n->target_count = (param <= 1.0) ? 1 : (uint32_t)param;
    return 0;
}

int stream_dag_process_sample(stream_dag_t *dag, uint16_t in_tag, double in_val, uint16_t *out_tag, double *out_val, bool *emitted) {
    if (!dag || !out_tag || !out_val || !emitted) return -1;
    *emitted = false;
    dag->total_processed++;

    for (uint32_t i = 0; i < dag->node_count; i++) {
        dag_node_t *n = &dag->nodes[i];
        if (n->src_tag != in_tag) continue;

        switch (n->op) {
            case DAG_OP_FILTER_GT:
                if (in_val > n->param) {
                    *out_tag = n->dst_tag;
                    *out_val = in_val;
                    *emitted = true;
                    dag->total_emitted++;
                    return 0;
                }
                break;
            case DAG_OP_FILTER_LT:
                if (in_val < n->param) {
                    *out_tag = n->dst_tag;
                    *out_val = in_val;
                    *emitted = true;
                    dag->total_emitted++;
                    return 0;
                }
                break;
            case DAG_OP_SCALE_OFFSET:
                *out_tag = n->dst_tag;
                *out_val = (in_val * n->param) + n->offset_param;
                *emitted = true;
                dag->total_emitted++;
                return 0;
            case DAG_OP_WINDOW_AVG:
                n->acc += in_val;
                n->current_count++;
                if (n->current_count >= n->target_count) {
                    *out_tag = n->dst_tag;
                    *out_val = n->acc / (double)n->current_count;
                    *emitted = true;
                    n->acc = 0.0;
                    n->current_count = 0;
                    dag->total_emitted++;
                    return 0;
                }
                break;
            case DAG_OP_WINDOW_SUM:
                n->acc += in_val;
                n->current_count++;
                if (n->current_count >= n->target_count) {
                    *out_tag = n->dst_tag;
                    *out_val = n->acc;
                    *emitted = true;
                    n->acc = 0.0;
                    n->current_count = 0;
                    dag->total_emitted++;
                    return 0;
                }
                break;
            default:
                break;
        }
    }

    return 0;
}

void stream_dag_get_stats(const stream_dag_t *dag, uint64_t *total_processed, uint64_t *total_emitted) {
    if (!dag) return;
    if (total_processed) *total_processed = dag->total_processed;
    if (total_emitted) *total_emitted = dag->total_emitted;
}
