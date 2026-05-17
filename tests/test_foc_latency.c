#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#include "stream_engine.h"

// Forward declaration
pipeline_stage_t *stage_foc_core_create(uint32_t output_tag, double v_dc, double dt, double kp_i, double ki_i);

static void test_foc_latency_benchmark(void **state) {
    (void)state;
    
    // Create FOC stage: Output Tag 7001, Vdc=24V, dt=100us, Kp=2.0, Ki=20.0
    pipeline_stage_t *foc = stage_foc_core_create(7001, 24.0, 0.0001, 2.0, 20.0);
    assert_non_null(foc);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_BATCH;
    ev.tag_id = 7000;
    ev.payload.batch.count = 5;
    double input_data[5] = {1.5, -0.5, 0.785, 0.0, 2.0}; // Ia, Ib, Theta, Id_ref, Iq_ref
    ev.payload.batch.ptr = input_data;

    const int iterations = 10000;
    uint64_t start = uv_hrtime();
    
    for (int i = 0; i < iterations; i++) {
        foc->process(foc, &ev);
    }
    
    uint64_t end = uv_hrtime();
    double total_us = (double)(end - start) / 1000.0;
    double avg_us = total_us / iterations;

    printf("FOC Core Latency Benchmark:\n");
    printf("  Total Time: %.2f ms for %d iterations\n", total_us / 1000.0, iterations);
    printf("  Average Latency: %.3f us\n", avg_us);

    // Acceptance Criteria: < 50us (Highly optimized)
    assert_true(avg_us < 50.0);

    // Cleanup
    if (foc->cleanup_state) foc->cleanup_state(foc->priv_state);
    free(foc);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_foc_latency_benchmark),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
