#ifndef ONNX_HOST_GUARD_H
#define ONNX_HOST_GUARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t dims[4];
    uint8_t num_dims;
    float scale;
    int32_t zero_point;
} onnx_tensor_meta_t;

typedef struct {
    onnx_tensor_meta_t input_meta;
    onnx_tensor_meta_t output_meta;
    uint64_t max_exec_time_ms;
    uint32_t total_inferences;
    uint32_t watchdog_timeouts;
} onnx_guard_ctx_t;

void onnx_guard_init(onnx_guard_ctx_t *ctx, float in_scale, int32_t in_zp, float out_scale, int32_t out_zp, uint64_t timeout_ms);
int onnx_quantize_input(const onnx_guard_ctx_t *ctx, const float *float_in, int8_t *q_out, size_t count);
int onnx_dequantize_output(const onnx_guard_ctx_t *ctx, const int8_t *q_in, float *float_out, size_t count);
int onnx_guard_run_inference(onnx_guard_ctx_t *ctx, uint64_t exec_duration_ms);
void onnx_guard_get_stats(const onnx_guard_ctx_t *ctx, uint32_t *total_inferences, uint32_t *timeouts);

#ifdef __cplusplus
}
#endif

#endif /* ONNX_HOST_GUARD_H */
