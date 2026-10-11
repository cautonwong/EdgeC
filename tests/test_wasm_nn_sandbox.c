/**
 * @file test_wasm_nn_sandbox.c
 * @brief Unit tests for Wasm-NN & Zero-Copy SHM Inference Sandbox (AEP-601)
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "edge_wasm_nn.h"

static void test_wasm_nn_tensor_view_zero_copy(void) {
    printf("[TEST] Wasm-NN SHM Zero-Copy Tensor View Binding...\n");

    /* Simulating DataHub SHM Ring buffer memory */
    float shm_buffer[1024];
    for (int i = 0; i < 1024; i++) {
        shm_buffer[i] = 220.0f + (float)i * 0.1f;
    }

    uint32_t dims[2] = {1, 1024};
    edge_tensor_view_t view;
    int ret = edge_wasm_nn_bind_tensor_view(&view, shm_buffer, sizeof(shm_buffer),
                                            TENSOR_DTYPE_FLOAT32, dims, 2);
    assert(ret == 0);
    assert(view.dim_count == 2);
    assert(view.dimensions[0] == 1);
    assert(view.dimensions[1] == 1024);
    assert(view.raw_data_ptr == (const void *)shm_buffer); /* 0-copy address match */
    assert(view.data_bytes == 1024 * sizeof(float));

    /* Buffer underflow test */
    ret = edge_wasm_nn_bind_tensor_view(&view, shm_buffer, 500, TENSOR_DTYPE_FLOAT32, dims, 2);
    assert(ret == -3);

    printf("  -> PASSED: Zero-copy SHM pointer binding and bounds check verified.\n");
}

static void test_wasm_nn_sandbox_inference_execution(void) {
    printf("[TEST] Wasm-NN Sandboxed Execution & Output Generation...\n");

    edge_wasm_nn_sandbox_t sandbox;
    int ret = edge_wasm_nn_sandbox_init(&sandbox, 1001, 20000); /* 20ms WCET */
    assert(ret == 0);

    float raw_inputs[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    uint32_t dims[1] = {10};
    edge_tensor_view_t view;
    edge_wasm_nn_bind_tensor_view(&view, raw_inputs, sizeof(raw_inputs), TENSOR_DTYPE_FLOAT32, dims,
                                  1);

    float out_score = 0.0f;
    size_t bytes_written = 0;
    ret = edge_wasm_nn_compute(&sandbox, &view, &out_score, sizeof(out_score), 500, &bytes_written);
    assert(ret == 0);
    assert(bytes_written == sizeof(float));
    assert(out_score > 5.4f && out_score < 5.6f); /* Average = 5.5f */
    assert(sandbox.telemetry.successful_inferences == 1);

    printf("  -> PASSED: Sandboxed execution produced correct result (5.5f).\n");
}

static void test_wasm_nn_wcet_watchdog_timeout_tripping(void) {
    printf("[TEST] Wasm-NN Watchdog WCET Timeout Isolation & Throttling...\n");

    edge_wasm_nn_sandbox_t sandbox;
    edge_wasm_nn_sandbox_init(&sandbox, 1002, 10000); /* 10ms budget */

    float raw_inputs[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    uint32_t dims[1] = {4};
    edge_tensor_view_t view;
    edge_wasm_nn_bind_tensor_view(&view, raw_inputs, sizeof(raw_inputs), TENSOR_DTYPE_FLOAT32, dims,
                                  1);

    float out_score = 0.0f;
    size_t bytes_written = 0;

    /* Simulate inference taking 15ms (> 10ms WCET budget) */
    int ret =
        edge_wasm_nn_compute(&sandbox, &view, &out_score, sizeof(out_score), 15000, &bytes_written);
    assert(ret == -2); /* Tripped */
    assert(sandbox.telemetry.watchdog_timeouts == 1);

    wasm_nn_telemetry_t telem;
    edge_wasm_nn_get_telemetry(&sandbox, &telem);
    assert(telem.watchdog_timeouts == 1);

    printf("  -> PASSED: Watchdog intercepted 15ms WCET overrun successfully.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-601: Wasm-NN SHM Zero-Copy Sandbox Test Suite          \n");
    printf("=================================================================\n");

    test_wasm_nn_tensor_view_zero_copy();
    test_wasm_nn_sandbox_inference_execution();
    test_wasm_nn_wcet_watchdog_timeout_tripping();

    printf("\nAll 3 AEP-601 Wasm-NN Sandbox test cases PASSED!\n");
    return 0;
}
