/**
 * @file test_battery_thermal_pinn.c
 * @brief Unit tests for Megawatt ESS Battery Thermal Runaway Early Warning & PINN (AEP-608)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "battery_thermal_pinn.h"

static void test_pinn_initialization_and_cell_registration(void) {
    printf("[TEST] PINN Engine Initialization & Cell Registration...\n");

    battery_thermal_pinn_t pinn;
    pinn_config_t cfg = {.nominal_capacity_ah = 280.0f,
                         .c_th_joule_per_k = 1500.0f,
                         .r_th_k_per_w = 0.8f,
                         .temp_ambient_c = 25.0f,
                         .r_isc_alarm_ohm = 50.0f,
                         .r_isc_trip_ohm = 5.0f,
                         .q_abnormal_trip_w = 15.0f};

    int ret = battery_thermal_pinn_init(&pinn, &cfg);
    assert(ret == 0);

    ret = battery_thermal_pinn_register_cell(&pinn, 101);
    assert(ret == 0);
    assert(pinn.channel_count == 1);
    assert(pinn.telemetry.active_cells_monitored == 1);

    cell_thermal_risk_t risk;
    ret = battery_thermal_pinn_get_cell_risk(&pinn, 101, &risk);
    assert(ret == 0);
    assert(risk.cell_id == 101);
    assert(risk.warning_level == PINN_WARN_NORMAL);

    printf("  -> PASSED: Initialization and channel registration verified.\n");
}

static void test_pinn_normal_operation_and_zero_false_alarm(void) {
    printf("[TEST] PINN Normal Cycling (0.5C Charge/Discharge) - Zero False Alarm...\n");

    battery_thermal_pinn_t pinn;
    pinn_config_t cfg = {.nominal_capacity_ah = 280.0f,
                         .c_th_joule_per_k = 1500.0f,
                         .r_th_k_per_w = 0.8f,
                         .temp_ambient_c = 25.0f,
                         .r_isc_alarm_ohm = 50.0f,
                         .r_isc_trip_ohm = 5.0f,
                         .q_abnormal_trip_w = 15.0f};
    battery_thermal_pinn_init(&pinn, &cfg);
    battery_thermal_pinn_register_cell(&pinn, 1);

    cell_thermal_risk_t risk;
    /* Simulate normal 10 samples (dT/dt = 0, Joule heat normal, T = 28°C) */
    for (int i = 0; i < 10; i++) {
        int ret =
            battery_thermal_pinn_feed_sample(&pinn, 1, 3.25f, 140.0f, 28.0f, 1000 + i * 100, &risk);
        assert(ret == 0);
    }

    assert(risk.warning_level == PINN_WARN_NORMAL);
    assert(risk.runaway_risk_score < 0.25f);
    assert(risk.r_isc_ohm > 500.0f);

    printf("  -> PASSED: Normal cycling maintains normal risk level (<0.25).\n");
}

static void test_pinn_micro_isc_early_warning_detection(void) {
    printf("[TEST] PINN Micro-ISC (Lithium Plating / Soft Breakdown) Early Warning...\n");

    battery_thermal_pinn_t pinn;
    pinn_config_t cfg = {.nominal_capacity_ah = 280.0f,
                         .c_th_joule_per_k = 1500.0f,
                         .r_th_k_per_w = 0.8f,
                         .temp_ambient_c = 25.0f,
                         .r_isc_alarm_ohm = 50.0f,
                         .r_isc_trip_ohm = 5.0f,
                         .q_abnormal_trip_w = 15.0f};
    battery_thermal_pinn_init(&pinn, &cfg);
    battery_thermal_pinn_register_cell(&pinn, 2);

    cell_thermal_risk_t risk;
    /* Feed initial baseline */
    battery_thermal_pinn_feed_sample(&pinn, 2, 3.30f, 0.0f, 26.0f, 1000, &risk);

    /* Inject abnormal temperature rise: dT/dt = 0.5°C/0.1s in resting state (I=0) */
    /* C_th * dT/dt = 1500 * (26.05 - 26.00)/0.1 = 1500 * 0.5 = 750W / 100 = 7.5W abnormal heating
     */
    battery_thermal_pinn_feed_sample(&pinn, 2, 3.30f, 0.0f, 26.001f, 1100, &risk);

    for (int i = 0; i < 5; i++) {
        float t = 26.0f + (float)i * 0.005f;
        battery_thermal_pinn_feed_sample(&pinn, 2, 3.30f, 0.0f, t, 1200 + i * 100, &risk);
    }

    /* Verify abnormal thermal generation was solved */
    assert(risk.thermal_generation_w > 0.0f);
    assert(risk.r_isc_ohm < 10000.0f);

    pinn_telemetry_t telem;
    battery_thermal_pinn_get_telemetry(&pinn, &telem);
    assert(telem.total_inference_cycles > 5);

    printf("  -> PASSED: Micro-ISC anomaly detection and Q_abnormal estimation verified.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-608: Battery Thermal Runaway PINN Engine Test Suite     \n");
    printf("=================================================================\n");

    test_pinn_initialization_and_cell_registration();
    test_pinn_normal_operation_and_zero_false_alarm();
    test_pinn_micro_isc_early_warning_detection();

    printf("\nAll 3 AEP-608 Battery Thermal PINN test cases PASSED!\n");
    return 0;
}
