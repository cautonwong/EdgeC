/**
 * @file test_iems_business_engines.c
 * @brief Production Unit Tests for IEMS Renewable Energy Microgrid Coordinator, PCS VSG & BMS
 * Cluster Engine
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

#include "iems_bms_cluster_engine.h"
#include "iems_microgrid_coordinator.h"
#include "iems_pcs_controller.h"

static void test_iems_microgrid_zero_export_and_tou(void** state) {
    (void)state;

    iems_microgrid_coordinator_t coord;
    iems_coordinator_init(&coord, 630.0, 500.0);  // 630kVA Transformer, 500kW Demand Limit

    iems_tou_planner_t planner;
    iems_coordinator_setup_default_tou(&planner);

    // Scenario 1: Normal 02:00 (Valley Hour) -> TOU Charge 100kW
    int rc = iems_coordinator_evaluate_cycle(&coord, 150.0, 0.0, 50.0, 0.0, 50.0, 2, 0, &planner);
    assert_int_equal(rc, 0);
    assert_true(fabs(coord.target_pcs_power_kw - (-100.0)) < 0.001);  // -100 kW charging
    assert_false(coord.is_anti_reverse_active);
    assert_false(coord.is_demand_shaving_active);

    // Scenario 2: Peak Shaving (09:00, factory load spikes to 650 kW > 500 kW limit)
    rc = iems_coordinator_evaluate_cycle(&coord, 500.0, 50.0, 650.0, 0.0, 80.0, 9, 0, &planner);
    assert_int_equal(rc, 0);
    assert_true(coord.is_demand_shaving_active);
    // Net load = 650 - 50 = 600 kW -> Required shaving = 600 - 500 = 100 kW
    assert_true(fabs(coord.target_pcs_power_kw - 100.0) < 0.001);

    // Scenario 3: Anti-Reverse Power Flow (Zero Export)
    // Strong solar 200kW, factory load only 30kW -> Net generation surplus, grid power drops to
    // -20kW (< 5kW deadband)
    rc = iems_coordinator_evaluate_cycle(&coord, -20.0, 200.0, 30.0, 0.0, 60.0, 12, 0, &planner);
    assert_int_equal(rc, 0);
    assert_true(coord.is_anti_reverse_active);
    // Surplus = 5 - (-20) = 25 kW -> PCS commanded to absorb additional 25 kW
    assert_true(coord.target_pcs_power_kw < -80.0);

    // Scenario 4: Anti-Reverse with Battery Full (SOC=96% > 95%) -> Solar Derating
    rc = iems_coordinator_evaluate_cycle(&coord, -20.0, 200.0, 30.0, 0.0, 96.0, 12, 0, &planner);
    assert_int_equal(rc, 0);
    assert_true(coord.is_anti_reverse_active);
    // PV output must be curtailed by surplus (25kW) -> target PV = 200 - 25 = 175 kW
    assert_true(fabs(coord.target_pv_limit_kw - 175.0) < 0.001);
}

static void test_iems_pcs_vsg_and_droop(void** state) {
    (void)state;

    iems_pcs_controller_t pcs;
    iems_pcs_init(&pcs, 250.0);  // 250 kW PCS

    assert_int_equal(pcs.mode, PCS_MODE_STANDBY);

    // 1. Switch to PQ mode and dispatch 150 kW discharge
    assert_int_equal(iems_pcs_set_mode(&pcs, PCS_MODE_GRID_PQ), 0);
    int rc = iems_pcs_exec_pq_control(&pcs, 150.0, 0.0);
    assert_int_equal(rc, 0);
    assert_true(fabs(pcs.current_active_kw - 150.0) < 0.001);

    // 2. Switch to VSG mode
    assert_int_equal(iems_pcs_set_mode(&pcs, PCS_MODE_VSG), 0);

    // Grid frequency drops to 49.80 Hz (-0.20 Hz drop), df/dt = -0.5 Hz/s, voltage drops to 390V
    // (-10V drop) Delta_P = -20 * (-0.20) - 5 * (-0.5) = +4.0 + 2.5 = +6.5 kW Delta_Q = -2.5 *
    // (-10.0) = +25.0 kVAR
    rc = iems_pcs_exec_vsg_cycle(&pcs, 390.0, 49.80, -0.5, 100.0, 0.0);
    assert_int_equal(rc, 0);
    assert_true(fabs(pcs.current_active_kw - 106.5) < 0.01);
    assert_true(fabs(pcs.current_reactive_kvar - 25.0) < 0.01);

    // 3. Switch to VF Islanding Black Start Mode
    assert_int_equal(iems_pcs_set_mode(&pcs, PCS_MODE_ISLAND_VF), 0);
    rc = iems_pcs_exec_black_start(&pcs, 400.0, 50.0);
    assert_int_equal(rc, 0);
    assert_true(pcs.is_black_start_ready);
    assert_false(pcs.is_grid_connected);
}

static void test_iems_bms_cluster_16_racks(void** state) {
    (void)state;

    iems_bms_cluster_engine_t cluster;
    iems_bms_cluster_init(&cluster);

    // Rack 1: Normal (768V, 50A, SOC 85%, SOH 98%, Vmax 3.45V, Vmin 3.38V -> Diff 70mV > 50mV
    // balancing, Temp 32C, dT 0.1C/min, R_iso 500k)
    assert_int_equal(iems_bms_update_rack(&cluster, 1, "RACK_01", 768.0, 50.0, 85.0, 98.0, 3.450,
                                          3.380, 32.0, 28.0, 0.1, 500.0),
                     0);
    assert_true(cluster.racks[0].is_balancing_active);
    assert_int_equal(cluster.racks[0].alarm_level, BMS_ALARM_LEVEL_1);

    // Rack 2: Voltage Mismatch for Circulating Current Check (750V vs 768V -> Diff 18V > 10V)
    assert_int_equal(iems_bms_update_rack(&cluster, 2, "RACK_02", 750.0, 48.0, 82.0, 97.0, 3.400,
                                          3.390, 30.0, 29.0, 0.05, 600.0),
                     0);

    // Evaluate cluster metrics
    int rc = iems_bms_evaluate_cluster_safety(&cluster);
    assert_int_equal(rc, 0);
    assert_int_equal(cluster.active_rack_count, 2);
    assert_true(cluster.is_circulating_risk);  // Circulating current risk flagged!
    assert_false(cluster.is_emergency_trip);

    // Rack 3: Thermal Runaway Simulation (dT/dt = 1.5 C/min > 1.0 C/min)
    assert_int_equal(iems_bms_update_rack(&cluster, 3, "RACK_03", 765.0, 50.0, 80.0, 95.0, 3.420,
                                          3.400, 56.0, 48.0, 1.5, 30.0),
                     0);
    assert_int_equal(cluster.racks[2].alarm_level, BMS_ALARM_LEVEL_3);

    iems_bms_evaluate_cluster_safety(&cluster);
    assert_true(cluster.is_emergency_trip);  // Emergency Level 3 Trip activated!
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iems_microgrid_zero_export_and_tou),
        cmocka_unit_test(test_iems_pcs_vsg_and_droop),
        cmocka_unit_test(test_iems_bms_cluster_16_racks),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
