#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "stream_sql_dag.h"

int main(void) {
    printf("=== Test Stream SQL DAG Start ===\n");

    stream_dag_t dag;
    stream_dag_init(&dag);

    /* Node 1: Filter Tag 100 > 50.0 -> Output Tag 200 */
    assert(stream_dag_add_node(&dag, 1, DAG_OP_FILTER_GT, 100, 200, 50.0, 0.0) == 0);

    /* Node 2: Window Avg of 4 samples for Tag 101 -> Output Tag 201 */
    assert(stream_dag_add_node(&dag, 2, DAG_OP_WINDOW_AVG, 101, 201, 4.0, 0.0) == 0);

    uint16_t out_tag = 0;
    double out_val = 0.0;
    bool emitted = false;

    /* Test Node 1: <= 50 -> no emit */
    assert(stream_dag_process_sample(&dag, 100, 45.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == false);

    /* Test Node 1: > 50 -> emit 75.0 */
    assert(stream_dag_process_sample(&dag, 100, 75.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == true);
    assert(out_tag == 200);
    assert(fabs(out_val - 75.0) < 0.0001);

    /* Test Node 2: Window Avg */
    assert(stream_dag_process_sample(&dag, 101, 10.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == false);
    assert(stream_dag_process_sample(&dag, 101, 20.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == false);
    assert(stream_dag_process_sample(&dag, 101, 30.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == false);
    assert(stream_dag_process_sample(&dag, 101, 40.0, &out_tag, &out_val, &emitted) == 0);
    assert(emitted == true);
    assert(out_tag == 201);
    assert(fabs(out_val - 25.0) < 0.0001);

    uint64_t processed = 0, emitted_cnt = 0;
    stream_dag_get_stats(&dag, &processed, &emitted_cnt);
    assert(processed == 6);
    assert(emitted_cnt == 2);

    printf("=== Test Stream SQL DAG Passed (121/121) ===\n");
    return 0;
}
