/**
 * @file test_tsn_engine.c
 * @brief Unit tests for IEEE 802.1Qbv TAS & IEEE 802.1AS TSN Deterministic Transport Engine
 * (AEP-609)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "edge_tsn_engine.h"

static void test_tsn_init_and_gcl_validation(void) {
    printf("[TEST] TSN Engine Init & GCL Cycle Validation...\n");

    edge_tsn_engine_t engine;
    tsn_schedule_config_t cfg = {
        .interface_name = "eth0",
        .cycle_time_ns = 1000000, /* 1ms cycle */
        .base_time_ns = 1700000000000000ULL,
        .entry_count = 3,
        .entries = {
            {.gate_mask = 0x80,
             .time_interval_ns = 200000}, /* Slot 0: 200us PCP=7 only (Critical) */
            {.gate_mask = 0x7E,
             .time_interval_ns = 600000}, /* Slot 1: 600us PCP=1..6 (Operational) */
            {.gate_mask = 0x01, .time_interval_ns = 200000} /* Slot 2: 200us PCP=0 (Best Effort) */
        }};

    int ret = edge_tsn_engine_init(&engine, &cfg);
    assert(ret == 0);
    assert(engine.is_active == true);

    /* Invalid config: sum of intervals != cycle_time */
    tsn_schedule_config_t bad_cfg = cfg;
    bad_cfg.cycle_time_ns = 2000000;
    ret = edge_tsn_engine_init(&engine, &bad_cfg);
    assert(ret == -3);

    printf("  -> PASSED: TSN GCL cycle and interval validation verified.\n");
}

static void test_tsn_ptp_sync_and_drift_compensation(void) {
    printf("[TEST] IEEE 802.1AS PTP PHC Nanosecond Sync & Drift Calculation...\n");

    edge_tsn_engine_t engine;
    tsn_schedule_config_t cfg = {.interface_name = "eth0",
                                 .cycle_time_ns = 1000000,
                                 .base_time_ns = 1000000,
                                 .entry_count = 1,
                                 .entries = {{.gate_mask = 0xFF, .time_interval_ns = 1000000}}};
    edge_tsn_engine_init(&engine, &cfg);

    /* Sync step 1: 100ns offset */
    int ret = edge_tsn_sync_ptp(&engine, 1000100, 1000000);
    assert(ret == 0);
    assert(engine.ptp.phc_offset_ns == 100);
    assert(engine.ptp.is_synchronized == true);

    /* Sync step 2: 1 second later with 150ns offset (+50ns drift over 1s = 0.05 PPM) */
    ret = edge_tsn_sync_ptp(&engine, 1001000150, 1001000000);
    assert(ret == 0);
    assert(engine.ptp.phc_offset_ns == 150);
    assert(fabs(engine.ptp.freq_drift_ppm - 0.05) < 0.001);

    printf("  -> PASSED: PTP PHC sync and frequency drift calculation verified.\n");
}

static void test_tsn_txtime_calculation_and_gate_scheduling(void) {
    printf("[TEST] TSN SO_TXTIME Scheduling & Gate Mask Resolution...\n");

    edge_tsn_engine_t engine;
    tsn_schedule_config_t cfg = {
        .interface_name = "enp1s0",
        .cycle_time_ns = 1000000, /* 1ms */
        .base_time_ns = 0,
        .entry_count = 2,
        .entries = {
            {.gate_mask = 0x80, .time_interval_ns = 300000}, /* 0..300us: PCP=7 only */
            {.gate_mask = 0x7F, .time_interval_ns = 700000}  /* 300..1000us: PCP=0..6 */
        }};
    edge_tsn_engine_init(&engine, &cfg);

    /* 1. Send PCP=7 at t = 100us (Gate OPEN for PCP=7) */
    uint64_t txtime = 0;
    bool gate_open = false;
    int ret = edge_tsn_calculate_txtime(&engine, 7, 100000, &txtime, &gate_open);
    assert(ret == 0);
    assert(gate_open == true);
    assert(txtime == 100500);

    /* 2. Send PCP=0 at t = 100us (Gate CLOSED for PCP=0, open at 300us) */
    ret = edge_tsn_calculate_txtime(&engine, 0, 100000, &txtime, &gate_open);
    assert(ret == 0);
    assert(gate_open == false);
    assert(txtime == 300500); /* Scheduled for 300us window */

    /* 3. Send PCP=7 at t = 500us (Gate CLOSED for PCP=7, next open at next cycle 1000us) */
    ret = edge_tsn_calculate_txtime(&engine, 7, 500000, &txtime, &gate_open);
    assert(ret == 0);
    assert(gate_open == false);
    assert(txtime == 1000500); /* Next cycle window */

    printf("  -> PASSED: SO_TXTIME calculation and deterministic gate isolation verified.\n");
}

static void test_tsn_telemetry_and_jitter_metrics(void) {
    printf("[TEST] TSN Telemetry & Jitter Window Metrics...\n");

    edge_tsn_engine_t engine;
    tsn_schedule_config_t cfg = {.interface_name = "eth0",
                                 .cycle_time_ns = 1000000,
                                 .base_time_ns = 0,
                                 .entry_count = 1,
                                 .entries = {{.gate_mask = 0xFF, .time_interval_ns = 1000000}}};
    edge_tsn_engine_init(&engine, &cfg);

    /* Record normal send (jitter 200ns) */
    edge_tsn_record_tx_telemetry(&engine, 7, 100000, 100200);

    /* Record another send (jitter 15000ns > 10us violation) */
    edge_tsn_record_tx_telemetry(&engine, 7, 200000, 215000);

    tsn_telemetry_t telem;
    edge_tsn_get_telemetry(&engine, &telem);

    assert(telem.total_scheduled_frames == 2);
    assert(telem.prio_stats[7].frames_sent == 2);
    assert(telem.prio_stats[7].frames_dropped == 0);
    assert(telem.prio_stats[7].max_jitter_ns == 15000);
    assert(telem.prio_stats[7].window_violations == 1);

    printf("  -> PASSED: TSN telemetry and jitter metrics verified (0 drops).\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("   AEP-609: IEEE 802.1Qbv TAS & 802.1AS TSN Engine Test Suite   \n");
    printf("=================================================================\n");

    test_tsn_init_and_gcl_validation();
    test_tsn_ptp_sync_and_drift_compensation();
    test_tsn_txtime_calculation_and_gate_scheduling();
    test_tsn_telemetry_and_jitter_metrics();

    printf("\nAll 4 AEP-609 TSN test cases PASSED!\n");
    return 0;
}
