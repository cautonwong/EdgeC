#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>
#include <unistd.h>

#include "wasm_mgr.h"
#include "stream_engine.h"

// Verified minimal WASM for "inference" function
static const uint8_t tinyml_dummy_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60, 
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0d, 0x01, 0x09, 0x69, 0x6e, 
    0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x00, 0x00, 0x0a, 0x04, 0x01, 
    0x02, 0x00, 0x0b
};

static void test_tinyml_invocation_overhead(void **state) {
    (void)state;
    
    const char *path = "tinyml_test.wasm";
    FILE *f = fopen(path, "wb");
    fwrite(tinyml_dummy_wasm, 1, sizeof(tinyml_dummy_wasm), f);
    fclose(f);

    wasm_mgr_init(NULL);

    // Create stage via V2-style definition
    edge_stage_def_t def = {
        .name = "ml_test",
        .type = "tinyml_wasm",
        .params = {
            .s1 = path,
            .s2 = "inference",
            .f1 = 1.0, // Window size 1
            .u1 = 8888 // Output tag
        }
    };
    
    extern pipeline_stage_t *stage_tinyml_wasm_create(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx);
    pipeline_stage_t *stage = stage_tinyml_wasm_create(&def, NULL);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;
    ev.payload.val.f = 23.4;

    const int iterations = 1000;
    uint64_t start = uv_hrtime();
    
    for (int i = 0; i < iterations; i++) {
        stage->process(stage, &ev);
    }
    
    uint64_t end = uv_hrtime();
    double total_ms = (double)(end - start) / 1000000.0;
    double avg_ms = total_ms / iterations;

    printf("TinyML WASM Invocation Benchmark:\n");
    printf("  Total Time: %.2f ms for %d iterations\n", total_ms, iterations);
    printf("  Average Overhead: %.3f ms\n", avg_ms);

    // Acceptance Criteria: < 5ms (Considering ASan + Threading Overhead)
    assert_true(avg_ms < 5.0);

    // Cleanup
    if (stage->cleanup_cfg) stage->cleanup_cfg(stage->priv_cfg);
    free(stage);
    wasm_mgr_cleanup();
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tinyml_invocation_overhead),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
