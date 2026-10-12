/**
 * @file edge_wasm_nn.c
 * @brief Wasm-NN & Shared Memory Zero-Copy Heterogeneous AI Inference Sandbox Implementation
 */

#include "edge_wasm_nn.h"
#include <string.h>

int edge_wasm_nn_sandbox_init(edge_wasm_nn_sandbox_t *sandbox, uint32_t graph_id, uint32_t wcet_budget_us) {
    if (!sandbox) return -1;
    memset(sandbox, 0, sizeof(edge_wasm_nn_sandbox_t));

    sandbox->graph_id = graph_id;
    sandbox->wcet_budget_us = (wcet_budget_us > 0) ? wcet_budget_us : WASM_NN_DEFAULT_WCET_US;
    sandbox->is_active = true;
    return 0;
}

int edge_wasm_nn_bind_tensor_view(edge_tensor_view_t *view, const void *shm_addr, size_t size,
                                  edge_tensor_dtype_t dtype, const uint32_t *dims, uint8_t dim_count) {
    if (!view || !shm_addr || size == 0 || !dims || dim_count == 0 || dim_count > WASM_NN_MAX_DIMS) {
        return -1;
    }

    /* Compute expected element count and byte size */
    size_t elem_count = 1;
    for (uint8_t i = 0; i < dim_count; i++) {
        if (dims[i] == 0) return -2;
        elem_count *= dims[i];
    }

    size_t elem_size = 4;
    switch (dtype) {
        case TENSOR_DTYPE_FLOAT32: elem_size = 4; break;
        case TENSOR_DTYPE_INT8:
        case TENSOR_DTYPE_UINT8:   elem_size = 1; break;
        case TENSOR_DTYPE_INT16:
        case TENSOR_DTYPE_FLOAT16: elem_size = 2; break;
    }

    size_t expected_bytes = elem_count * elem_size;
    if (size < expected_bytes) return -3; /* Buffer underflow */

    memset(view, 0, sizeof(edge_tensor_view_t));
    for (uint8_t i = 0; i < dim_count; i++) {
        view->dimensions[i] = dims[i];
    }
    view->dim_count = dim_count;
    view->dtype = dtype;
    view->raw_data_ptr = shm_addr; /* Direct pointer, 0-copy */
    view->data_bytes = expected_bytes;
    return 0;
}

int edge_wasm_nn_compute(edge_wasm_nn_sandbox_t *sandbox, const edge_tensor_view_t *input,
                         void *out_buffer, size_t out_buffer_cap,
                         uint32_t sim_exec_time_us, size_t *out_bytes_written) {
    if (!sandbox || !sandbox->is_active || !input || !out_buffer) return -1;

    sandbox->telemetry.total_inferences++;

    /* 1. Watchdog Isolation & WCET Budget Enforcement */
    if (sim_exec_time_us > sandbox->wcet_budget_us) {
        sandbox->telemetry.watchdog_timeouts++;
        sandbox->telemetry.last_execution_time_us = sim_exec_time_us;
        return -2; /* WCET Timeout tripped */
    }

    /* 2. Fault containment: check memory safety & null pointers */
    if (!input->raw_data_ptr || input->data_bytes == 0) {
        sandbox->telemetry.faults_intercepted++;
        return -3; /* Fault intercepted */
    }

    /* 3. Execute lightweight model inference in sandbox:
     * In this prototype, perform a 1D moving average / activation score
     */
    float anomaly_score = 0.0f;
    if (input->dtype == TENSOR_DTYPE_FLOAT32) {
        const float *farr = (const float *)input->raw_data_ptr;
        size_t count = input->data_bytes / sizeof(float);
        float sum = 0.0f;
        for (size_t i = 0; i < count; i++) {
            sum += farr[i];
        }
        anomaly_score = (count > 0) ? (sum / (float)count) : 0.0f;
    } else if (input->dtype == TENSOR_DTYPE_INT8) {
        const int8_t *iarr = (const int8_t *)input->raw_data_ptr;
        size_t count = input->data_bytes;
        int32_t sum = 0;
        for (size_t i = 0; i < count; i++) {
            sum += iarr[i];
        }
        anomaly_score = (count > 0) ? ((float)sum / (float)count) : 0.0f;
    }

    if (out_buffer_cap < sizeof(float)) return -4;
    memcpy(out_buffer, &anomaly_score, sizeof(float));
    if (out_bytes_written) *out_bytes_written = sizeof(float);

    sandbox->telemetry.successful_inferences++;
    sandbox->telemetry.last_execution_time_us = sim_exec_time_us;
    if (sim_exec_time_us > sandbox->telemetry.max_execution_time_us) {
        sandbox->telemetry.max_execution_time_us = sim_exec_time_us;
    }

    return 0;
}

void edge_wasm_nn_get_telemetry(const edge_wasm_nn_sandbox_t *sandbox, wasm_nn_telemetry_t *out_telem) {
    if (sandbox && out_telem) {
        memcpy(out_telem, &sandbox->telemetry, sizeof(wasm_nn_telemetry_t));
    }
}
