/**
 * @file test_facility_coordinator.c
 * @brief Unit tests for Facility Microgrid & Environment Coordinator Operator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>

#include "facility_coordinator.h"

static void test_facility_coordinator_init_null(void **state) {
    (void)state;
    facility_control_cmd_t cmd;
    assert_int_equal(facility_coordinator_init(NULL), -1);
    assert_int_equal(facility_coordinator_init(&cmd), 0);
    assert_float_equal(cmd.hvac_fan_speed_pct, 30.0f, 0.01f);
}

static void test_facility_coordinator_workflow(void **state) {
    (void)state;
    facility_control_cmd_t cmd;
    assert_int_equal(facility_coordinator_init(&cmd), 0);

    /* Normal grid demand & good CO2 */
    facility_telemetry_t telem1 = {
        .grid_power_kw = 40.0f,
        .battery_soc_pct = 50.0f,
        .indoor_temp_c = 24.0f,
        .indoor_co2_ppm = 600.0f,
        .peak_shave_threshold_kw = 50.0f
    };

    assert_int_equal(facility_coordinator_step(&telem1, &cmd), 0);
    assert_false(cmd.peak_shaving_active);
    assert_float_equal(cmd.hvac_fan_speed_pct, 30.0f, 0.01f);

    /* Peak demand Spike & High CO2 */
    facility_telemetry_t telem2 = {
        .grid_power_kw = 65.0f,
        .battery_soc_pct = 80.0f,
        .indoor_temp_c = 26.0f,
        .indoor_co2_ppm = 1200.0f,
        .peak_shave_threshold_kw = 50.0f
    };

    assert_int_equal(facility_coordinator_step(&telem2, &cmd), 0);
    assert_true(cmd.peak_shaving_active);
    assert_float_equal(cmd.battery_charge_cmd_kw, -15.0f, 0.01f); /* Discharge 15kW */
    assert_true(cmd.fresh_air_boost);
    assert_float_equal(cmd.hvac_fan_speed_pct, 100.0f, 0.01f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_facility_coordinator_init_null),
        cmocka_unit_test(test_facility_coordinator_workflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
