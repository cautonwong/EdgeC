#ifndef STREAM_SQL_DAG_H
#define STREAM_SQL_DAG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAG_MAX_NODES 32

typedef enum {
    DAG_OP_FILTER_GT = 1,   /* Pass if input > param */
    DAG_OP_FILTER_LT = 2,   /* Pass if input < param */
    DAG_OP_WINDOW_AVG = 3,  /* Compute window avg over N samples (param = sample count) */
    DAG_OP_WINDOW_SUM = 4,  /* Compute window sum over N samples */
    DAG_OP_SCALE_OFFSET = 5 /* val * param + offset */
} dag_op_type_t;

typedef struct {
    uint32_t node_id;
    dag_op_type_t op;
    uint16_t src_tag;
    uint16_t dst_tag;
    double param;
    double offset_param;
    double acc;
    uint32_t current_count;
    uint32_t target_count;
} dag_node_t;

typedef struct {
    dag_node_t nodes[DAG_MAX_NODES];
    uint32_t node_count;
    uint64_t total_processed;
    uint64_t total_emitted;
} stream_dag_t;

void stream_dag_init(stream_dag_t *dag);
int stream_dag_add_node(stream_dag_t *dag, uint32_t node_id, dag_op_type_t op, uint16_t src_tag, uint16_t dst_tag, double param, double offset_param);
int stream_dag_process_sample(stream_dag_t *dag, uint16_t in_tag, double in_val, uint16_t *out_tag, double *out_val, bool *emitted);
void stream_dag_get_stats(const stream_dag_t *dag, uint64_t *total_processed, uint64_t *total_emitted);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_SQL_DAG_H */
