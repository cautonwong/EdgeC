/**
 * @file test_cdd_tensor_schema.c
 * @brief Unit tests for CDD Multi-Modal Tensor Pipeline & DAG Schema (AEP-603)
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "edge_tensor_pipeline.h"

static void test_tensor_pipeline_dag_initialization(void) {
    printf("[TEST] CDD Tensor Pipeline DAG Initialization & Topology...\n");

    tensor_domain_def_t def = {.domain_name = "vibration_3axis_highfreq",
                               .domain_id = 101,
                               .sample_rate_hz = 25600,
                               .window_size = 1024,
                               .stride = 512,
                               .shape = {1, 1024, 3},
                               .quant_scale = 0.003f,
                               .quant_zero_point = 0,
                               .stage_count = 3,
                               .stages = {{.stage_id = 1,
                                           .stage_type = TENSOR_STAGE_DSP_WINDOW_HANNING,
                                           .depends_on_stage_id = 0,
                                           .wcet_limit_us = 2000},
                                          {.stage_id = 2,
                                           .stage_type = TENSOR_STAGE_FFT_SPECTRUM,
                                           .depends_on_stage_id = 1,
                                           .wcet_limit_us = 5000},
                                          {.stage_id = 3,
                                           .stage_type = TENSOR_STAGE_TINYML_INFERENCE,
                                           .depends_on_stage_id = 2,
                                           .wcet_limit_us = 8000}}};

    edge_tensor_pipeline_t pipe;
    int ret = edge_tensor_pipeline_init(&pipe, &def);
    assert(ret == 0);
    assert(pipe.is_valid_dag == true);

    /* Verify 64-byte alignment of tensor slice buffer */
    uintptr_t addr = (uintptr_t)pipe.slice.buffer;
    assert((addr % TENSOR_SIMD_ALIGNMENT) == 0);

    printf("  -> PASSED: Linear DAG initialized and 64-byte SIMD alignment verified.\n");
}

static void test_tensor_pipeline_circular_dag_detection(void) {
    printf("[TEST] CDD Tensor Pipeline Circular Dependency Assertion...\n");

    /* Create circular DAG: Stage 1 -> Stage 2 -> Stage 1 */
    tensor_domain_def_t bad_def = {
        .domain_name = "circular_pipeline",
        .domain_id = 999,
        .stage_count = 2,
        .stages = {
            {.stage_id = 1,
             .stage_type = TENSOR_STAGE_DSP_WINDOW_HANNING,
             .depends_on_stage_id = 2},
            {.stage_id = 2, .stage_type = TENSOR_STAGE_FFT_SPECTRUM, .depends_on_stage_id = 1}}};

    bool has_cycle = edge_tensor_pipeline_has_circular_dependency(&bad_def);
    assert(has_cycle == true);

    edge_tensor_pipeline_t pipe;
    int ret = edge_tensor_pipeline_init(&pipe, &bad_def);
    assert(ret == -3); /* Circular DAG rejected */

    printf("  -> PASSED: Circular DAG dependency (1 -> 2 -> 1) correctly rejected.\n");
}

static void test_tensor_pipeline_frame_processing(void) {
    printf("[TEST] CDD Tensor Pipeline Waveform Frame Processing...\n");

    tensor_domain_def_t def = {
        .domain_name = "acoustic_emission",
        .domain_id = 102,
        .sample_rate_hz = 50000,
        .window_size = 512,
        .stride = 256,
        .shape = {1, 512, 1},
        .stage_count = 3,
        .stages = {
            {.stage_id = 1, .stage_type = TENSOR_STAGE_FFT_SPECTRUM, .depends_on_stage_id = 0},
            {.stage_id = 2, .stage_type = TENSOR_STAGE_TINYML_INFERENCE, .depends_on_stage_id = 1},
            {.stage_id = 3, .stage_type = TENSOR_STAGE_THRESHOLD_ALARM, .depends_on_stage_id = 2}}};

    edge_tensor_pipeline_t pipe;
    edge_tensor_pipeline_init(&pipe, &def);

    /* 1. Normal acoustic frame (amplitude < 500) */
    int16_t normal_samples[256];
    for (int i = 0; i < 256; i++) normal_samples[i] = 100;

    bool alarm = false;
    int ret = edge_tensor_pipeline_process_frame(&pipe, normal_samples, sizeof(normal_samples),
                                                 1000, &alarm);
    assert(ret == 0);
    assert(alarm == false);

    /* 2. Shock anomaly acoustic frame (amplitude 15000 > 10000) */
    int16_t shock_samples[256];
    for (int i = 0; i < 256; i++) shock_samples[i] = 15000;

    ret = edge_tensor_pipeline_process_frame(&pipe, shock_samples, sizeof(shock_samples), 2000,
                                             &alarm);
    assert(ret == 0);
    assert(alarm == true);
    assert(pipe.total_alarms_triggered == 1);
    assert(pipe.total_frames_processed == 2);

    printf("  -> PASSED: Pipeline execution correctly detected shock anomaly.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-603: CDD Tensor Pipeline & DAG Schema Test Suite        \n");
    printf("=================================================================\n");

    test_tensor_pipeline_dag_initialization();
    test_tensor_pipeline_circular_dag_detection();
    test_tensor_pipeline_frame_processing();

    printf("\nAll 3 AEP-603 Tensor Pipeline test cases PASSED!\n");
    return 0;
}
