#include "onnx_host_guard.h"
#include <string.h>
#include <math.h>

void onnx_guard_init(onnx_guard_ctx_t *ctx, float in_scale, int32_t in_zp, float out_scale, int32_t out_zp, uint64_t timeout_ms) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(onnx_guard_ctx_t));
    ctx->input_meta.scale = (in_scale == 0.0f) ? 1.0f : in_scale;
    ctx->input_meta.zero_point = in_zp;
    ctx->output_meta.scale = (out_scale == 0.0f) ? 1.0f : out_scale;
    ctx->output_meta.zero_point = out_zp;
    ctx->max_exec_time_ms = timeout_ms;
}

int onnx_quantize_input(const onnx_guard_ctx_t *ctx, const float *float_in, int8_t *q_out, size_t count) {
    if (!ctx || !float_in || !q_out) return -1;
    float scale = ctx->input_meta.scale;
    int32_t zp = ctx->input_meta.zero_point;

    for (size_t i = 0; i < count; i++) {
        int32_t val = (int32_t)roundf(float_in[i] / scale) + zp;
        if (val > 127) val = 127;
        if (val < -128) val = -128;
        q_out[i] = (int8_t)val;
    }
    return 0;
}

int onnx_dequantize_output(const onnx_guard_ctx_t *ctx, const int8_t *q_in, float *float_out, size_t count) {
    if (!ctx || !q_in || !float_out) return -1;
    float scale = ctx->output_meta.scale;
    int32_t zp = ctx->output_meta.zero_point;

    for (size_t i = 0; i < count; i++) {
        float_out[i] = ((float)q_in[i] - (float)zp) * scale;
    }
    return 0;
}

int onnx_guard_run_inference(onnx_guard_ctx_t *ctx, uint64_t exec_duration_ms) {
    if (!ctx) return -1;
    ctx->total_inferences++;

    if (ctx->max_exec_time_ms > 0 && exec_duration_ms > ctx->max_exec_time_ms) {
        ctx->watchdog_timeouts++;
        return -2; /* Execution Timeout / Watchdog Kill */
    }

    return 0;
}

void onnx_guard_get_stats(const onnx_guard_ctx_t *ctx, uint32_t *total_inferences, uint32_t *timeouts) {
    if (!ctx) return;
    if (total_inferences) *total_inferences = ctx->total_inferences;
    if (timeouts) *timeouts = ctx->watchdog_timeouts;
}
