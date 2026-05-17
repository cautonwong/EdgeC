#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "stream_engine.h"
#include "edge_linalg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration
pipeline_stage_t *stage_ahrs_quaternion_create(uint32_t gyro_tag, uint32_t output_tag);

typedef struct {
    pipeline_stage_t *stage;
} test_ctx_t;

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(test_ctx_t));
    ctx->stage = stage_ahrs_quaternion_create(100, 200);
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *ctx = *state;
    if (ctx->stage->cleanup_state) ctx->stage->cleanup_state(ctx->stage->priv_state);
    free(ctx->stage);
    free(ctx);
    return 0;
}

static void test_ahrs_quaternion_gimbal_lock_free(void **state) {
    test_ctx_t *ctx = *state;
    pipeline_stage_t *stage = ctx->stage;
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    double vec_data[4]; 
    ev.type = PIPEMSG_TYPE_BATCH;
    ev.payload.batch.ptr = vec_data;

    uint64_t current_ts = 0;
    int steps = 100;

    // 1. Rotate 90 degrees around X-axis (gx = pi/2)
    double gx = (M_PI / 2.0);
    for (int i = 0; i < steps; i++) {
        ev.tag_id = 100; // Reset tag ID
        ev.payload.batch.count = 3;
        vec_data[0] = gx;
        vec_data[1] = 0.0;
        vec_data[2] = 0.0;
        ev.ts = current_ts;
        current_ts += 10; // 10ms
        
        stage->process(stage, &ev);
    }
    
    // Check intermediate quaternion after 90 deg X rotation
    // Expected: w=cos(45)=0.7071, x=sin(45)=0.7071, y=0, z=0
    double w = vec_data[0];
    double x = vec_data[1];
    double y = vec_data[2];
    double z = vec_data[3];
    printf("After 90 deg X: q = [%.4f, %.4f, %.4f, %.4f]\n", w, x, y, z);
    assert_float_equal(w, 0.7071, 0.01);
    assert_float_equal(x, 0.7071, 0.01);
    assert_float_equal(y, 0.0, 0.01);
    assert_float_equal(z, 0.0, 0.01);

    // 2. Rotate 90 degrees around local Y-axis
    double gy = (M_PI / 2.0);
    for (int i = 0; i < steps; i++) {
        ev.tag_id = 100; // Reset tag ID
        ev.payload.batch.count = 3;
        vec_data[0] = 0.0;
        vec_data[1] = gy;
        vec_data[2] = 0.0;
        ev.ts = current_ts;
        current_ts += 10;
        
        stage->process(stage, &ev);
    }

    w = vec_data[0];
    x = vec_data[1];
    y = vec_data[2];
    z = vec_data[3];
    printf("After 90 deg local Y: q = [%.4f, %.4f, %.4f, %.4f]\n", w, x, y, z);
    
    // Expected final: q_x * q_y = [0.707, 0.707, 0, 0] * [0.707, 0, 0.707, 0]
    // w = 0.5 - 0 = 0.5
    // x = 0 + 0.5 = 0.5
    // y = 0.5 + 0 = 0.5
    // z = 0.5 - 0 = 0.5
    assert_float_equal(w, 0.5, 0.01);
    assert_float_equal(x, 0.5, 0.01);
    assert_float_equal(y, 0.5, 0.01);
    assert_float_equal(z, 0.5, 0.01);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_ahrs_quaternion_gimbal_lock_free, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
