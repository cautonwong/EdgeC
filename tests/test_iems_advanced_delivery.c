/**
 * @file test_iems_advanced_delivery.c
 * @brief Production Unit Tests for Advanced IEMS Multi-PCS Dispatcher, Fire Interlock & Site
 * Commissioning Engine
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "iems_commissioning_tool.h"
#include "iems_fire_safety_interlock.h"
#include "iems_multi_pcs_dispatcher.h"

static void test_iems_multi_pcs_load_sharing(void** state) {
    (void)state;

    iems_multi_pcs_dispatcher_t disp;
    iems_pcs_dispatcher_init(&disp, 1000.0);  // 1000 kW/s ramp rate

    // Register 4 PCS units (250 kW each = 1000 kW capacity)
    assert_int_equal(iems_pcs_dispatcher_register_unit(&disp, 1, "PCS_01", 250.0), 0);
    assert_int_equal(iems_pcs_dispatcher_register_unit(&disp, 2, "PCS_02", 250.0), 0);
    assert_int_equal(iems_pcs_dispatcher_register_unit(&disp, 3, "PCS_03", 250.0), 0);
    assert_int_equal(iems_pcs_dispatcher_register_unit(&disp, 4, "PCS_04", 250.0), 0);
    assert_int_equal(disp.unit_count, 4);
    assert_true(fabs(disp.total_rated_capacity_kw - 1000.0) < 0.001);

    // 1. Equal load sharing: Demand 600 kW -> 150 kW per unit
    int rc = iems_pcs_dispatcher_dispatch(&disp, 600.0, 1.0);
    assert_int_equal(rc, 0);
    for (int i = 0; i < 4; i++) {
        assert_true(fabs(disp.units[i].target_power_kw - 150.0) < 0.001);
    }

    // 2. Thermal Derating on PCS 2 (Temp 80℃ -> Derating factor 0.5)
    assert_int_equal(iems_pcs_dispatcher_update_status(&disp, 2, true, false, 150.0, 80.0), 0);
    assert_true(fabs(disp.units[1].derating_factor - 0.5) < 0.001);

    // Re-dispatch 600 kW -> Effective capacities: PCS1=250, PCS2=125, PCS3=250, PCS4=250 -> Total
    // 875 kW PCS2 share = 125/875 = 1/7 -> target = 600 / 7 = 85.71 kW PCS1,3,4 share = 250/875 =
    // 2/7 -> target = 1200 / 7 = 171.43 kW
    rc = iems_pcs_dispatcher_dispatch(&disp, 600.0, 1.0);
    assert_int_equal(rc, 0);
    assert_true(fabs(disp.units[1].target_power_kw - (600.0 * 125.0 / 875.0)) < 0.01);
    assert_true(fabs(disp.units[0].target_power_kw - (600.0 * 250.0 / 875.0)) < 0.01);

    // 3. Fault on PCS 4 -> Excluded from dispatch
    assert_int_equal(iems_pcs_dispatcher_update_status(&disp, 4, true, true, 0.0, 45.0), 0);
    rc = iems_pcs_dispatcher_dispatch(&disp, 500.0, 1.0);
    assert_int_equal(rc, 0);
    assert_true(fabs(disp.units[3].target_power_kw - 0.0) < 0.001);
}

static void test_iems_fire_safety_3_stages(void** state) {
    (void)state;

    iems_fire_safety_interlock_t fire;
    iems_fire_interlock_init(&fire);

    assert_int_equal(fire.current_stage, FIRE_STAGE_NORMAL);

    // 1. Stage 1 (Warning): CO = 35 ppm -> warning + exhaust fan ON
    int rc = iems_fire_interlock_evaluate(&fire, 35.0, 10.0, 25.0, false, false, 0.1);
    assert_int_equal(rc, 0);
    assert_int_equal(fire.current_stage, FIRE_STAGE_1_WARNING);
    assert_true(fire.is_sound_light_alarm);
    assert_true(fire.is_ventilation_fan_on);
    assert_false(fire.is_pcs_emergency_stop);
    assert_false(fire.is_hv_breaker_tripped);

    // 2. Stage 2 (Alarm): Smoke detected -> Stop PCS + close fire dampers
    rc = iems_fire_interlock_evaluate(&fire, 40.0, 50.0, 30.0, true, false, 0.1);
    assert_int_equal(rc, 0);
    assert_int_equal(fire.current_stage, FIRE_STAGE_2_ALARM);
    assert_true(fire.is_pcs_emergency_stop);
    assert_true(fire.is_fire_damper_closed);
    assert_false(fire.is_ventilation_fan_on);
    assert_false(fire.is_gas_released);

    // 3. Stage 3 (Release): Dual Confirmation (CO 120ppm + Smoke) -> Trip HV Breaker + Release
    rc = iems_fire_interlock_evaluate(&fire, 120.0, 100.0, 60.0, true, false, 0.1);
    assert_int_equal(rc, 0);
    assert_int_equal(fire.current_stage, FIRE_STAGE_3_RELEASE);
    assert_true(fire.is_hv_breaker_tripped);
    assert_true(fire.is_pcs_emergency_stop);
    assert_true(fire.is_gas_released);
}

static void test_iems_site_commissioning_preflight(void** state) {
    (void)state;

    // 1. Test CT/PT Transformer scaling
    iems_meter_transformer_cfg_t cfg;
    iems_commissioning_set_transformer(&cfg, 1000, 5, 10000, 100);
    assert_true(fabs(cfg.calculated_ct_ratio - 200.0) < 0.001);
    assert_true(fabs(cfg.calculated_pt_ratio - 100.0) < 0.001);
    assert_true(fabs(cfg.total_power_multiplier - 20000.0) < 0.001);

    double scaled_power = iems_commissioning_scale_power(&cfg, 5.0);  // 5W raw -> 100kW
    assert_true(fabs(scaled_power - 100000.0) < 0.001);

    // 2. Pre-Flight Check (All Healthy)
    iems_commissioning_report_t report;
    int rc = iems_commissioning_run_preflight_check(&report, 395.0, 50.01, 800.0, 4.5, false, false,
                                                    0.05);
    assert_int_equal(rc, 0);
    assert_true(report.is_go_for_grid_tie);
    assert_int_equal(report.total_score, 100);

    // 3. Pre-Flight Check (Low Insulation Fault -> No-Go)
    rc = iems_commissioning_run_preflight_check(&report, 395.0, 50.01, 200.0, 4.5, false, false,
                                                0.05);
    assert_int_equal(rc, 0);
    assert_false(report.is_go_for_grid_tie);
    assert_false(report.is_bms_insulation_ok);
    assert_true(report.total_score < 100);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iems_multi_pcs_load_sharing),
        cmocka_unit_test(test_iems_fire_safety_3_stages),
        cmocka_unit_test(test_iems_site_commissioning_preflight),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
