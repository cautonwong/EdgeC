#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "stream_engine.h"
#include "radar_algo.h"

// Forward declaration
pipeline_stage_t *stage_radar_tracker_create(uint32_t output_tag);

static void test_radar_tracker_basic(void **state) {
    (void)state;
    
    // Create stage: Output Tag 5001
    pipeline_stage_t *stage = stage_radar_tracker_create(5001);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    double detections[4]; // 2 targets: (x1, y1), (x2, y2)
    ev.type = PIPEMSG_TYPE_BATCH;
    ev.tag_id = 1001;
    ev.payload.batch.ptr = detections;
    ev.payload.batch.count = 4;

    // Frame 1: 2 detections
    detections[0] = 10.0; detections[1] = 20.0;
    detections[2] = -5.0; detections[3] = -10.0;
    ev.ts = 1000;
    
    // First frame should return STAGE_DROP as we need 2 hits for a stable track
    assert_int_equal(stage->process(stage, &ev), STAGE_DROP);
    
    // Frame 2: Same targets slightly moved
    detections[0] = 10.5; detections[1] = 20.5;
    detections[2] = -5.5; detections[3] = -10.5;
    ev.ts = 1100;
    ev.payload.batch.count = 4;
    
    assert_int_equal(stage->process(stage, &ev), STAGE_OK);
    assert_int_equal(ev.payload.batch.count, 4); // 2 targets * 2
    
    double *out = (double *)ev.payload.batch.ptr;
    printf("Target 1: x=%.2f, y=%.2f\n", out[0], out[1]);
    printf("Target 2: x=%.2f, y=%.2f\n", out[2], out[3]);
    
    // Target 1 was (10.0, -5.0) -> (10.5, -5.5)
    assert_true(fabs(out[0] - 10.25) < 0.5);
    assert_true(fabs(out[1] + 5.25) < 0.5);
    
    // Target 2 was (20.0, -10.0) -> (20.5, -10.5)
    assert_true(fabs(out[2] - 20.25) < 0.5);
    assert_true(fabs(out[3] + 10.25) < 0.5);

    // Cleanup
    if (stage->cleanup_state) stage->cleanup_state(stage->priv_state);
    free(stage);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_radar_tracker_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
