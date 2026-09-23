/**
 * @file test_dcu_business_engines.c
 * @brief Production Unit Tests for DCU Substation Grid Analytics, Line Loss, Load Control, Outage,
 * Vertical & Horizontal Tasks, SOE Alarms, Telecontrol, & Data Fusion
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

#include "dcu_data_fusion_engine.h"
#include "dcu_horizontal_tasks.h"
#include "dcu_load_control.h"
#include "dcu_outage_detector.h"
#include "dcu_peripheral_telecontrol.h"
#include "dcu_soe_event_engine.h"
#include "dcu_topology_lineloss.h"
#include "dcu_vertical_tasks.h"

static void test_dcu_topology_and_lineloss(void** state) {
    (void)state;

    dcu_topology_engine_t engine;
    dcu_topology_init(&engine);

    // Register 3 single-phase meters
    assert_int_equal(dcu_topology_update_meter(&engine, "10000001", 1, 101, PHASE_A, 120.0, 220.0,
                                               5.0, 0.95, 25),
                     0);
    assert_int_equal(dcu_topology_update_meter(&engine, "10000002", 2, 101, PHASE_B, 115.0, 221.0,
                                               4.8, 0.94, 28),
                     0);
    assert_int_equal(dcu_topology_update_meter(&engine, "10000003", 3, 102, PHASE_C, 118.0, 219.0,
                                               5.2, 0.96, 22),
                     0);

    assert_int_equal(engine.meter_count, 3);

    // Total consumed = 120 + 115 + 118 = 353 kWh
    // Master input = 365 kWh -> Loss = 12 kWh -> Loss rate = (12 / 365) * 100% = 3.287% (NORMAL <=
    // 4%)
    uint64_t now_ms = 1750000000000ULL;
    int rc = dcu_topology_compute_analytics(&engine, 365.0, 15.0, 14.8, 15.2, 220.0, 221.0, 219.0,
                                            now_ms);
    assert_int_equal(rc, 0);

    assert_true(fabs(engine.analytics.total_consumed_kwh - 353.0) < 0.001);
    assert_true(fabs(engine.analytics.line_loss_kwh - 12.0) < 0.001);
    assert_true(fabs(engine.analytics.line_loss_rate_pct - 3.2876) < 0.01);
    assert_int_equal(engine.analytics.health, LINELOSS_HEALTH_NORMAL);
    assert_int_equal(engine.analytics.online_meter_count, 3);
    assert_int_equal(engine.analytics.offline_meter_count, 0);

    // Test 3-Phase Current Unbalance
    assert_true(fabs(engine.analytics.current_unbalance_pct - 1.333) < 0.01);

    // Test High Loss scenario
    dcu_topology_compute_analytics(&engine, 450.0, 20.0, 15.0, 15.0, 220.0, 220.0, 220.0,
                                   now_ms + 60000);
    assert_int_equal(engine.analytics.health, LINELOSS_HEALTH_HIGH_LOSS);

    // Test Negative Loss scenario
    dcu_topology_compute_analytics(&engine, 300.0, 12.0, 12.0, 12.0, 220.0, 220.0, 220.0,
                                   now_ms + 120000);
    assert_int_equal(engine.analytics.health, LINELOSS_HEALTH_NEGATIVE);
}

static void test_dcu_load_control_and_demand_response(void** state) {
    (void)state;

    dcu_load_controller_t ctrl;
    dcu_load_control_init(&ctrl, 630.0);  // 630 kVA Rated Transformer

    // Scenario 1: Normal Load
    dcu_load_state_t st = dcu_load_control_evaluate(&ctrl, 400.0, 600.0, 0.95, 10);
    assert_int_equal(st, LOAD_STATE_NORMAL);
    assert_false(ctrl.is_curtailment_active);

    // Scenario 2: Warning Level 1
    st = dcu_load_control_evaluate(&ctrl, 400.0, 800.0, 0.95, 10);
    assert_int_equal(st, LOAD_STATE_WARNING);
    assert_true(ctrl.is_curtailment_active);
    assert_true(ctrl.target_reduction_kw > 0.0);

    // Scenario 3: High Level 2
    st = dcu_load_control_evaluate(&ctrl, 400.0, 900.0, 0.95, 10);
    assert_int_equal(st, LOAD_STATE_HIGH);
    assert_true(ctrl.is_curtailment_active);

    // Scenario 4: Overload Emergency Level 3
    st = dcu_load_control_evaluate(&ctrl, 400.0, 1050.0, 0.95, 10);
    assert_int_equal(st, LOAD_STATE_OVERLOAD);
    assert_true(ctrl.is_curtailment_active);
    assert_int_equal(ctrl.overload_duration_s, 10);
}

static void test_dcu_outage_detector_and_last_gasp(void** state) {
    (void)state;

    dcu_outage_detector_t detector;
    dcu_outage_detector_init(&detector);

    uint64_t t0 = 1750000000000ULL;

    // Normal supply
    dcu_outage_status_t st =
        dcu_outage_detector_evaluate(&detector, 220.0, 220.0, 220.0, 220.0, 5200, t0);
    assert_int_equal(st, OUTAGE_STATUS_NORMAL);
    assert_int_equal(detector.total_outage_events, 0);

    // Single phase loss
    st = dcu_outage_detector_evaluate(&detector, 5.0, 220.0, 220.0, 220.0, 5100, t0 + 1000);
    assert_int_equal(st, OUTAGE_STATUS_SINGLE_PHASE);
    assert_int_equal(detector.total_outage_events, 1);

    // Full blackout
    st = dcu_outage_detector_evaluate(&detector, 2.0, 3.0, 1.5, 220.0, 4800, t0 + 2000);
    assert_int_equal(st, OUTAGE_STATUS_FULL_BLACKOUT);
    assert_int_equal(detector.supercap_voltage_mv, 4800);
    assert_int_equal(detector.outage_start_timestamp_ms, t0 + 2000);

    // Power restoration
    st = dcu_outage_detector_evaluate(&detector, 220.0, 220.0, 220.0, 220.0, 5000, t0 + 15000);
    assert_int_equal(st, OUTAGE_STATUS_NORMAL);
    assert_int_equal(detector.last_restoration_timestamp_ms, t0 + 15000);
}

static void test_dcu_vertical_tasks_lifecycle(void** state) {
    (void)state;

    dcu_vertical_task_mgr_t* mgr =
        (dcu_vertical_task_mgr_t*)calloc(1, sizeof(dcu_vertical_task_mgr_t));
    assert_non_null(mgr);
    dcu_vertical_mgr_init(mgr);

    dcu_meter_vertical_state_t* m = dcu_vertical_get_or_create_meter(mgr, "10000001", 1);
    assert_non_null(m);
    m->daily_freeze.energy_total_kwh = 100.0;

    uint64_t now_ms = 1750000000000ULL;

    // 1. Ingest 1s samples
    int rc = dcu_vertical_1s_sample(mgr, 1, 220.5, 5.2, 1.146, 50.01, now_ms);
    assert_int_equal(rc, 0);
    assert_true(fabs(m->last_voltage_v - 220.5) < 0.001);
    assert_true(fabs(m->daily_freeze.max_demand_kw - 1.146) < 0.001);

    // 2. Perform 15-min periodic freeze
    int frozen = dcu_vertical_15min_freeze(mgr, now_ms + 900000);
    assert_int_equal(frozen, 1);
    assert_int_equal(m->curve_point_count, 1);
    assert_true(fabs(m->load_curve[0].avg_active_power_kw - 1.146) < 0.001);

    // 3. Perform daily TOU freeze (00:00:00)
    frozen = dcu_vertical_daily_freeze(mgr, now_ms + 86400000);
    assert_int_equal(frozen, 1);
    assert_true(fabs(m->daily_freeze.energy_t1_peak_kwh - 30.0) < 0.001);
    assert_true(fabs(m->daily_freeze.energy_t2_shoulder_kwh - 35.0) < 0.001);
    assert_true(fabs(m->daily_freeze.energy_t3_flat_kwh - 20.0) < 0.001);
    assert_true(fabs(m->daily_freeze.energy_t4_valley_kwh - 15.0) < 0.001);

    // 4. Perform monthly billing freeze
    frozen = dcu_vertical_monthly_freeze(mgr, now_ms + 2592000000ULL);
    assert_int_equal(frozen, 1);
    assert_true(fabs(m->monthly_freeze.energy_total_kwh - 100.0) < 0.001);

    free(mgr);
}

static void test_dcu_horizontal_tasks_spatial_and_vvc(void** state) {
    (void)state;

    dcu_horizontal_engine_t h_engine;
    dcu_horizontal_init(&h_engine);

    // 1. Spatial line loss
    int rc = dcu_horizontal_compute_spatial_loss(&h_engine, 1, "Branch_01_North", 100.0, 98.0, 30);
    assert_int_equal(rc, 0);
    assert_false(h_engine.branches[0].is_abnormal);

    rc = dcu_horizontal_compute_spatial_loss(&h_engine, 2, "Branch_02_Market", 100.0, 85.0, 45);
    assert_int_equal(rc, 0);
    assert_true(h_engine.branches[1].is_abnormal);
    assert_true(fabs(h_engine.branches[1].branch_loss_rate_pct - 15.0) < 0.01);

    // 2. 3-Phase Dynamic Load Balancing
    uint64_t now_ms = 1750000000000ULL;
    int recommend = dcu_horizontal_evaluate_phase_balancing(&h_engine, 100.0, 50.0, 40.0, 105,
                                                            "10000105", 1, 25.0, now_ms);
    assert_int_equal(recommend, 1);
    assert_int_equal(h_engine.last_commutation.target_meter_dev_id, 105);
    assert_int_equal(h_engine.last_commutation.from_phase, 1);
    assert_int_equal(h_engine.last_commutation.to_phase, 3);
    assert_true(fabs(h_engine.last_commutation.expected_shift_current_a - 25.0) < 0.001);

    // 3. Volt-VAR Control (VVC)
    rc = dcu_horizontal_compute_volt_var(&h_engine, 225.0, 200.0, 50.0);
    assert_int_equal(rc, 0);
    assert_true(h_engine.vvc_state.is_vvc_active);
    assert_true(fabs(h_engine.vvc_state.required_q_inject_kvar - 50.0) < 0.01);
}

static void test_dcu_soe_event_engine(void** state) {
    (void)state;

    dcu_soe_event_engine_t soe;
    dcu_soe_init(&soe);

    uint64_t t0 = 1750000000000ULL;

    // Trigger Overvoltage Alarm (3000)
    int rc = dcu_soe_record_event(&soe, OAD_EVT_OVERVOLTAGE, "10000001", 1, EVT_STATE_TRIGGERED,
                                  265.5, t0, "Phase A Overvoltage 265.5V");
    assert_int_equal(rc, 0);
    assert_int_equal(dcu_soe_get_active_alarms(&soe), 1);
    assert_int_equal(soe.records[0].oad_event_code, 0x3000);
    assert_true(fabs(soe.records[0].trigger_value - 265.5) < 0.001);

    // Trigger Overtemperature Alarm (3015)
    rc = dcu_soe_record_event(&soe, OAD_EVT_TEMP_EXCEEDED, "10000001", 1, EVT_STATE_TRIGGERED, 95.0,
                              t0 + 500, "Terminal Temp 95C Exceeded");
    assert_int_equal(rc, 0);
    assert_int_equal(dcu_soe_get_active_alarms(&soe), 2);

    // Restore Overvoltage
    rc = dcu_soe_record_event(&soe, OAD_EVT_OVERVOLTAGE, "10000001", 1, EVT_STATE_RESTORED, 220.0,
                              t0 + 2000, "Voltage Restored to 220V");
    assert_int_equal(rc, 0);
    assert_int_equal(dcu_soe_get_active_alarms(&soe), 1);
}

// Mock I/O callbacks for telecontrol testing
static int mock_read_di(uint8_t ch, bool* out, void* ctx) {
    (void)ctx;
    *out = (ch == 0) ? true : false;  // Breaker closed by default
    return 0;
}
static int mock_set_do(uint8_t ch, bool state, void* ctx) {
    (void)ch;
    (void)state;
    (void)ctx;
    return 0;
}
static int mock_set_oltc(int8_t tap, void* ctx) {
    (void)tap;
    (void)ctx;
    return 0;
}

static void test_dcu_peripheral_telecontrol(void** state) {
    (void)state;

    dcu_io_ops_t ops = {.read_di = mock_read_di,
                        .set_do = mock_set_do,
                        .set_oltc_tap = mock_set_oltc,
                        .user_data = NULL};

    dcu_telecontrol_manager_t tc;
    dcu_telecontrol_init(&tc, &ops);

    // 1. Scan YX
    int rc = dcu_telecontrol_scan_yx(&tc);
    assert_int_equal(rc, 0);
    assert_int_equal(tc.breaker_state, BREAKER_STATE_CLOSED);

    // 2. YK Remote Trip (Open Breaker)
    rc = dcu_telecontrol_exec_yk(&tc, 0, YK_ACTION_TRIP);
    assert_int_equal(rc, 0);
    assert_int_equal(tc.breaker_state, BREAKER_STATE_OPEN);

    // 3. Switch Capacitor Bank 1 ON
    rc = dcu_telecontrol_switch_capacitor(&tc, 1, true);
    assert_int_equal(rc, 0);
    assert_true(tc.capacitor_bank_states[1]);

    // 4. OLTC Tap Regulation (+2 step)
    rc = dcu_telecontrol_regulate_oltc(&tc, 2);
    assert_int_equal(rc, 0);
    assert_int_equal(tc.current_oltc_tap, 2);
}

static void test_dcu_data_fusion_engine(void** state) {
    (void)state;

    dcu_data_fusion_engine_t* fusion =
        (dcu_data_fusion_engine_t*)calloc(1, sizeof(dcu_data_fusion_engine_t));
    assert_non_null(fusion);
    dcu_fusion_init(fusion);

    // Meter 1: Normal consumer (220V, 5.0kW, SNR 25dB, RSSI -65dBm)
    assert_int_equal(dcu_fusion_update_meter_sources(fusion, "10000001", 1, 220.0, 5.0, 25, -65),
                     0);
    // Meter 2: Suspected theft (219V, 0.0kW, high SNR 28dB, heavy grid load)
    assert_int_equal(dcu_fusion_update_meter_sources(fusion, "10000002", 2, 219.0, 0.0, 28, -60),
                     0);

    uint64_t now_ms = 1750000000000ULL;
    int rc = dcu_fusion_process_analytics(fusion, 220.0, 250.0, now_ms);
    assert_int_equal(rc, 0);

    assert_int_equal(fusion->theft_suspect_count, 1);
    assert_false(fusion->meters[0].is_theft_suspect);
    assert_true(fusion->meters[1].is_theft_suspect);
    assert_int_equal(fusion->meters[0].identified_phase, 1);  // Phase A

    free(fusion);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_topology_and_lineloss),
        cmocka_unit_test(test_dcu_load_control_and_demand_response),
        cmocka_unit_test(test_dcu_outage_detector_and_last_gasp),
        cmocka_unit_test(test_dcu_vertical_tasks_lifecycle),
        cmocka_unit_test(test_dcu_horizontal_tasks_spatial_and_vvc),
        cmocka_unit_test(test_dcu_soe_event_engine),
        cmocka_unit_test(test_dcu_peripheral_telecontrol),
        cmocka_unit_test(test_dcu_data_fusion_engine),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
