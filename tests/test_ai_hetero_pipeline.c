/**
 * @file test_ai_hetero_pipeline.c
 * @brief Unit tests for Heterogeneous Pipeline Orchestrator (AEP-710 / Issue #243)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_hetero_pipeline.h"

static void test_hetero_pipeline_subgraph_partitioning(void **state) {
    (void)state;

    ai_hetero_pipeline_storage_t storage;
    ai_hetero_pipeline_t *pipe = ai_hetero_pipeline_create_in(&storage);
    assert_non_null(pipe);

    /* 6-Operator sequence:
     * Op 0: DSP Filter (DSP) - 50 us
     * Op 1: FFT (DSP) - 80 us
     * Op 2: Conv1D (NPU) - 120 us
     * Op 3: Dense GEMM (NPU) - 200 us
     * Op 4: Post-Process Threshold (CPU) - 30 us
     * Op 5: Alert Generation (CPU) - 20 us
     */
    ai_hetero_op_spec_t ops[6] = {
        {.op_id = 0, .preferred_backend = AI_BACKEND_DSP, .estimated_cost_us = 50},
        {.op_id = 1, .preferred_backend = AI_BACKEND_DSP, .estimated_cost_us = 80},
        {.op_id = 2, .preferred_backend = AI_BACKEND_NPU, .estimated_cost_us = 120},
        {.op_id = 3, .preferred_backend = AI_BACKEND_NPU, .estimated_cost_us = 200},
        {.op_id = 4, .preferred_backend = AI_BACKEND_CPU, .estimated_cost_us = 30},
        {.op_id = 5, .preferred_backend = AI_BACKEND_CPU, .estimated_cost_us = 20}};

    assert_int_equal(ai_hetero_pipeline_partition(pipe, ops, 6), 0);

    ai_hetero_pipeline_metrics_t metrics;
    assert_true(ai_hetero_pipeline_get_metrics(pipe, &metrics));

    /* Should be partitioned into 3 subgraphs (DSP, NPU, CPU) */
    assert_int_equal(metrics.num_subgraphs, 3);
    assert_int_equal(metrics.num_dma_transfers, 2);

    /* Sequential time = (50+80) + (120+200) + (30+20) = 130 + 320 + 50 = 500 us */
    assert_int_equal(metrics.sequential_time_us, 500);

    /* Max stage = 320 us, DMA overhead = 2 * 10 = 20 us -> Pipelined time = 340 us */
    assert_int_equal(metrics.pipelined_time_us, 340);

    /* Overlap = 100 * (1 - 340 / 500) = 32.0 % */
    assert_float_equal(metrics.pipeline_overlap_pct, 32.0f, 0.1f);

    assert_int_equal(ai_hetero_pipeline_execute_step(pipe), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hetero_pipeline_subgraph_partitioning),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
