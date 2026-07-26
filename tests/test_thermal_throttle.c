/**
 * @file test_thermal_throttle.c
 * @brief Unit tests for Dynamic Thermal Throttle & Power Governor Operator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>

#include "thermal_throttle.h"

static void test_thermal_governor_init_null(void **state) {
    (void)state;
    assert_int_equal(thermal_governor_init(NULL), -1);
}

static void test_thermal_governor_thresholds(void **state) {
    (void)state;
    thermal_governor_t gov;
    assert_int_equal(thermal_governor_init(&gov), 0);
    assert_int_equal(gov.state, THERMAL_STATE_NORMAL);
    assert_int_equal(gov.cpu_freq_limit_mhz, 1500);

    /* Normal state */
    assert_int_equal(thermal_governor_update(&gov, 55.0f, 40.0f, 12.0f), 0);
    assert_int_equal(gov.state, THERMAL_STATE_NORMAL);

    /* Warning state */
    assert_int_equal(thermal_governor_update(&gov, 68.0f, 50.0f, 12.0f), 0);
    assert_int_equal(gov.state, THERMAL_STATE_WARNING);
    assert_int_equal(gov.cpu_freq_limit_mhz, 1200);

    /* Throttle state */
    assert_int_equal(thermal_governor_update(&gov, 78.0f, 70.0f, 12.0f), 0);
    assert_int_equal(gov.state, THERMAL_STATE_THROTTLE);
    assert_int_equal(gov.cpu_freq_limit_mhz, 1000);
    assert_int_equal(gov.sample_decimation_factor, 2);

    /* Critical state */
    assert_int_equal(thermal_governor_update(&gov, 88.0f, 96.0f, 12.0f), 0);
    assert_int_equal(gov.state, THERMAL_STATE_CRITICAL);
    assert_int_equal(gov.cpu_freq_limit_mhz, 600);
    assert_int_equal(gov.sample_decimation_factor, 4);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_thermal_governor_init_null),
        cmocka_unit_test(test_thermal_governor_thresholds),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
