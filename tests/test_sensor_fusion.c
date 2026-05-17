#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "stream_engine.h"

// Forward declaration
pipeline_stage_t *stage_sensor_fusion_create(uint32_t imu_tag, uint32_t gps_tag, uint32_t out_tag);

typedef struct {
    pipeline_stage_t *stage;
} test_ctx_t;

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(test_ctx_t));
    ctx->stage = stage_sensor_fusion_create(100, 200, 300);
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

static void test_sensor_fusion_imu_gps(void **state) {
    test_ctx_t *ctx = *state;
    pipeline_stage_t *stage = ctx->stage;

    pipe_msg_t ev = {0};
    double vec_data[5]; 

    // 1. Initial State: Pos=0, Vel=0
    // Push IMU: Accel = 1.0 m/s^2 for 1 second (10 steps of 0.1s)
    for (int i = 1; i <= 10; i++) {
        ev.type = PIPEMSG_TYPE_BATCH;
        ev.tag_id = 100; // IMU
        ev.payload.batch.count = 1;
        ev.payload.batch.ptr = vec_data;
        vec_data[0] = 1.0; // 1 m/s^2
        ev.ts = (uint64_t)(i * 100); 
        
        stage->process(stage, &ev);
    }
    
    // 2. Push GPS: Observed Pos = 0.55m
    ev.type = PIPEMSG_TYPE_BATCH;
    ev.tag_id = 200; // GPS
    ev.payload.batch.count = 1;
    ev.payload.batch.ptr = vec_data;
    vec_data[0] = 0.55;
    ev.ts = 1000;
    
    assert_int_equal(stage->process(stage, &ev), STAGE_OK);
    
    double fused_pos = vec_data[0];
    double fused_vel = vec_data[1];
    printf("Fused State after 1s: Pos=%.3f, Vel=%.2f\n", fused_pos, fused_vel);
    
    // Range check (Relaxed for EKF convergence)
    assert_true(fused_pos > 0.3 && fused_pos < 0.8);
    assert_true(fused_vel > 0.5 && fused_vel < 1.5);

    // 3. Loss of GPS: Continue IMU for another 1s
    for (int i = 11; i <= 20; i++) {
        ev.tag_id = 100;
        vec_data[0] = 1.0; 
        ev.ts = (uint64_t)(i * 100);
        stage->process(stage, &ev);
    }
    
    // Trigger correction with mock GPS to read state
    ev.tag_id = 200;
    vec_data[0] = 2.0; 
    stage->process(stage, &ev);
    
    fused_pos = vec_data[0];
    printf("Fused State after another 1s (Constant Accel): Pos=%.3f\n", fused_pos);
    
    // Expected around 2.0m. Predicted (no GPS) should be roughly 0.5 + 1.0*1 + 0.5*1*1 = 2.0
    assert_true(fused_pos > 1.5 && fused_pos < 2.5);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_sensor_fusion_imu_gps, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
