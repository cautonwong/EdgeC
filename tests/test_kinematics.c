#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

// Forward declaration
pipeline_stage_t* stage_kinematics_create(uint32_t output_tag, double l1, double l2);

static void test_kinematics_ik_scara(void** state) {
    (void)state;

    // Create stage: Output Tag 8888, l1=0.4, l2=0.3
    pipeline_stage_t* stage = stage_kinematics_create(8888, 0.4, 0.3);
    assert_non_null(stage);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_BATCH;
    ev.tag_id = 1001;
    ev.payload.batch.count = 4;
    double input_data[4] = {0.7, 0.0, 0.1, 0.0};  // X, Y, Z, Yaw
    ev.payload.batch.ptr = input_data;

    // 1. Fully extended (0.4 + 0.3 = 0.7)
    assert_int_equal(stage->process(stage, &ev), STAGE_OK);
    // q1=0, q2=0
    assert_float_equal(input_data[0], 0.0, 0.001);
    assert_float_equal(input_data[1], 0.0, 0.001);
    assert_float_equal(input_data[2], 0.1, 0.001);
    assert_float_equal(input_data[3], 0.0, 0.001);

    // 2. Folded (X=0.4, Y=0.3)
    // l1=0.4, l2=0.3. Triangle 0.4, 0.3, dist=0.5
    // q2: cos(q2) = (0.5^2 - 0.4^2 - 0.3^2) / (2 * 0.4 * 0.3) = (0.25 - 0.16 - 0.09) / 0.24 = 0 /
    // 0.24 = 0 q2 = 90 deg
    input_data[0] = 0.4;
    input_data[1] = 0.3;
    input_data[2] = 0.2;
    input_data[3] = 0.0;
    ev.payload.batch.count = 4;

    assert_int_equal(stage->process(stage, &ev), STAGE_OK);

    printf("SCARA IK Result (X=0.4, Y=0.3): q1=%.2f, q2=%.2f, q3=%.2f, q4=%.2f\n", input_data[0],
           input_data[1], input_data[2], input_data[3]);

    assert_float_equal(input_data[1], 90.0, 0.001);
    // q1 should be atan2(0.3, 0.4) - atan2(0.3, 0.4) = 0?
    // Actually, atan2(0.3, 0.4) is approx 36.87 deg.
    // k1 = 0.4 + 0.3*0 = 0.4. k2 = 0.3*1 = 0.3. atan2(0.3, 0.4) is 36.87.
    // q1 = 36.87 - 36.87 = 0.
    assert_float_equal(input_data[0], 0.0, 0.001);
    assert_float_equal(input_data[2], 0.2, 0.001);

    // Cleanup
    if (stage->cleanup_state) stage->cleanup_state(stage->priv_state);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_kinematics_ik_scara),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
