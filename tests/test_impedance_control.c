/**
 * @file test_impedance_control.c
 * @brief Unit tests for Rehabilitation Robotic Impedance Control Operator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>
#include <string.h>

#include "impedance_control.h"

static void test_impedance_control_init_null(void **state) {
    (void)state;
    impedance_params_t params;
    assert_int_equal(impedance_control_init(NULL, 1.0f, 10.0f, 100.0f, 0.5f, 50.0f), -1);
    assert_int_equal(impedance_control_init(&params, 0.0f, 10.0f, 100.0f, 0.5f, 50.0f), -1);
}

static void test_impedance_control_loop(void **state) {
    (void)state;
    impedance_params_t params;
    assert_int_equal(impedance_control_init(&params, 2.0f, 5.0f, 100.0f, 1.0f, 40.0f), 0);

    impedance_state_t st;
    memset(&st, 0, sizeof(impedance_state_t));
    st.current_pos = 0.8f; /* 0.2m error from target 1.0m */
    st.current_vel = 0.1f;
    st.external_force = 2.0f;

    assert_int_equal(impedance_control_step(&params, &st, 0.001f), 0);

    /* Spring force = 100 * (1.0 - 0.8) = 20.0 N */
    /* Damping force = -5 * 0.1 = -0.5 N */
    /* External force = 2.0 N */
    /* Total desired force = 20.0 - 0.5 + 2.0 = 21.5 N*m */
    assert_float_equal(st.output_torque, 21.5f, 0.01f);
    assert_true(st.active_assistance);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_impedance_control_init_null),
        cmocka_unit_test(test_impedance_control_loop),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
