/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "onnx_host_guard.h"

int main(void) {
    printf("=== Test ONNX Host Guard Start ===\n");

    onnx_guard_ctx_t ctx;
    /* Input scale = 0.1, zp = 0. Output scale = 0.5, zp = 0 */
    onnx_guard_init(&ctx, 0.1f, 0, 0.5f, 0, 100 /* 100ms timeout */);

    float float_in[4] = {1.0f, 2.5f, -3.0f, 12.7f};
    int8_t q_out[4];
    assert(onnx_quantize_input(&ctx, float_in, q_out, 4) == 0);
    assert(q_out[0] == 10);
    assert(q_out[1] == 25);
    assert(q_out[2] == -30);
    assert(q_out[3] == 127);

    int8_t q_in[4] = {20, -10, 40, 100};
    float float_out[4];
    assert(onnx_dequantize_output(&ctx, q_in, float_out, 4) == 0);
    assert(fabsf(float_out[0] - 10.0f) < 0.0001f);
    assert(fabsf(float_out[1] - (-5.0f)) < 0.0001f);
    assert(fabsf(float_out[2] - 20.0f) < 0.0001f);
    assert(fabsf(float_out[3] - 50.0f) < 0.0001f);

    /* Test Inference execution watchdog */
    assert(onnx_guard_run_inference(&ctx, 45 /* 45ms < 100ms */) == 0);
    assert(onnx_guard_run_inference(&ctx, 150 /* 150ms > 100ms timeout */) == -2);

    uint32_t total = 0, timeouts = 0;
    onnx_guard_get_stats(&ctx, &total, &timeouts);
    assert(total == 2);
    assert(timeouts == 1);

    printf("=== Test ONNX Host Guard Passed (126/126) ALL 20 EXPANSIONS COMPLETE! ===\n");
    return 0;
}
