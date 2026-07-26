/**
 * @file test_gait_safety.c
 * @brief Unit tests for Multimodal Gait Analysis & Fall Active Safety Operator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>
#include <string.h>

#include "gait_safety.h"

static void test_gait_safety_init_null(void **state) {
    (void)state;
    gait_safety_status_t status;
    assert_int_equal(gait_safety_init(NULL), -1);
    assert_int_equal(gait_safety_init(&status), 0);
}

static void test_gait_safety_eval_normal_and_hazard(void **state) {
    (void)state;
    gait_safety_status_t status;
    assert_int_equal(gait_safety_init(&status), 0);

    /* Normal posture input */
    gait_sensory_input_t normal_in = {
        .cop_x_cm = 1.0f,
        .cop_y_cm = 1.5f,
        .body_tilt_deg = 2.0f,
        .tilt_velocity = 5.0f,
        .harness_tension_n = 20.0f
    };

    assert_int_equal(gait_safety_eval(&normal_in, &status), 0);
    assert_false(status.fall_hazard_detected);
    assert_false(status.emergency_lock_triggered);
    assert_true(status.hazard_score < 0.3f);

    /* Fall Hazard input (Sudden tilt and velocity) */
    gait_sensory_input_t hazard_in = {
        .cop_x_cm = 8.0f,
        .cop_y_cm = 10.0f,
        .body_tilt_deg = 30.0f,
        .tilt_velocity = 95.0f,
        .harness_tension_n = 180.0f
    };

    assert_int_equal(gait_safety_eval(&hazard_in, &status), 0);
    assert_true(status.fall_hazard_detected);
    assert_true(status.emergency_lock_triggered);
    assert_true(status.hazard_score > 0.6f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_gait_safety_init_null),
        cmocka_unit_test(test_gait_safety_eval_normal_and_hazard),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
