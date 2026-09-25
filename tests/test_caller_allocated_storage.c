/**
 * @file test_caller_allocated_storage.c
 * @brief Unit Tests for Caller-Allocated Storage, Opaque Encapsulation & Aggregate Roots (AEP-752 /
 * Issue #340)
 *
 * Verifies:
 * 1. Physical Opaque Encapsulation (Opaque struct pointer + aligned storage placeholder);
 * 2. In-place placement construction (*_create_in) with zero heap allocation;
 * 3. Consumer-Defined Interfaces & Ports injection across Aggregate Roots;
 * 4. Symmetric Composition Root lifecycle with Caller-Allocated Storage;
 * 5. State invariant preservation and deterministic step execution.
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
#include <string.h>

#include "battery_balancer.h"
#include "battery_eis.h"
#include "battery_fo_ecm_ekf.h"
#include "battery_gas_early_warning.h"
#include "battery_micro_isc.h"
#include "battery_pulse_heating.h"
#include "battery_soh_fusion.h"
#include "edge_composition_root.h"
#include "mpc_dispatcher.h"
#include "vsg_control.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_battery_ekf_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    fo_ecm_ekf_storage_t ekf_store;
    memset(&ekf_store, 0xCC, sizeof(ekf_store));

    fo_ecm_params_t params = {.alpha = 0.75,
                              .r0 = 0.005,
                              .rp = 0.008,
                              .c_alpha = 1200.0,
                              .capacity_ah = 100.0,
                              .coulombic_eff = 0.998,
                              .temp_coeff_r0 = 0.002,
                              .temp_ref_c = 25.0};

    /* 2. In-place construct aggregate root */
    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&ekf_store, &params, 0.85, 25.0, 1.0);
    assert_non_null(ekf);
    assert_ptr_equal(ekf, (void *)ekf_store.raw);

    /* 3. Verify state invariants via opaque accessors */
    assert_float_equal(fo_ecm_ekf_get_soc(ekf), 0.85, 0.001);
    assert_float_equal(fo_ecm_ekf_get_up(ekf), 0.0, 0.001);
    assert_float_equal(fo_ecm_ekf_get_gl_weight(ekf, 0), 1.0, 0.001);

    /* 4. Execute time step */
    assert_true(fo_ecm_ekf_step(ekf, -20.0, 3.30, 25.0, 1.0));
    assert_true(fo_ecm_ekf_get_soc(ekf) > 0.0 && fo_ecm_ekf_get_soc(ekf) <= 1.0);
    assert_int_equal(fo_ecm_ekf_get_step_count(ekf), 1);

    /* 5. Reset */
    fo_ecm_ekf_reset(ekf, 0.50, 30.0);
    assert_float_equal(fo_ecm_ekf_get_soc(ekf), 0.50, 0.001);
}

static void test_vsg_controller_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    vsg_controller_storage_t vsg_store;
    memset(&vsg_store, 0xAA, sizeof(vsg_store));

    vsg_config_t cfg = {.j_inertia = 2.0,
                        .d_damping = 20.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 50000.0, /* 50 kW */
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3};

    /* 2. Setup mock consumer ports */
    edge_consumer_ports_t mock_ports;
    assert_int_equal(edge_port_create_mock_ports(&mock_ports, 0.0), 0);
    vsg_ports_t ports = {
        .reader = mock_ports.reader, .writer = mock_ports.writer, .clock = mock_ports.clock};

    /* 3. In-place construct VSG aggregate root */
    vsg_controller_t *vsg = vsg_controller_create_in(&vsg_store, &cfg, &ports);
    assert_non_null(vsg);
    assert_ptr_equal(vsg, (void *)vsg_store.raw);

    assert_float_equal(vsg_controller_get_frequency(vsg), 50.0, 0.001);
    assert_float_equal(vsg_controller_get_voltage(vsg), 220.0, 0.001);

    /* 4. Step VSG swing equation under load step (P_meas = 55 kW) */
    double out_f = 0.0, out_v = 0.0;
    assert_int_equal(vsg_controller_step(vsg, 55000.0, 5000.0, 0.01, &out_f, &out_v), 0);
    assert_true(out_f > 45.0 && out_f < 55.0);
    assert_true(out_v > 180.0 && out_v < 260.0);

    /* 5. Step via Consumer Ports */
    uint32_t tag_p = 301;
    uint32_t tag_q = 302;
    uint32_t tag_f_out = 303;
    uint32_t tag_v_out = 304;

    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_p, 50000.0, 1000ULL);
    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_q, 0.0, 1000ULL);

    assert_int_equal(vsg_controller_step_ports(vsg, tag_p, tag_q, tag_f_out, tag_v_out, 0.01), 0);

    double read_freq = 0.0;
    mock_ports.reader.read_tag(mock_ports.reader.ctx, tag_f_out, &read_freq, NULL);
    assert_true(read_freq > 45.0 && read_freq < 55.0);
}

static void test_mpc_dispatcher_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate storage on stack (0 malloc) */
    mpc_dispatcher_storage_t mpc_store;
    memset(&mpc_store, 0x55, sizeof(mpc_store));

    mpc_dispatcher_config_t cfg = {.horizon = 5,
                                   .dt_s = 1.0,
                                   .capacity_ah = 100.0,
                                   .target_soc = 0.60,
                                   .q_soc_weight = 1000.0,
                                   .r_power_weight = 0.1,
                                   .max_power_kw = 50.0,
                                   .min_power_kw = -50.0};

    /* 2. Setup mock ports */
    edge_consumer_ports_t mock_ports;
    assert_int_equal(edge_port_create_mock_ports(&mock_ports, 0.0), 0);
    mpc_dispatcher_ports_t ports = {
        .reader = mock_ports.reader, .writer = mock_ports.writer, .clock = mock_ports.clock};

    /* 3. In-place construct MPC aggregate root */
    mpc_dispatcher_t *mpc = mpc_dispatcher_create_in(&mpc_store, &cfg, &ports);
    assert_non_null(mpc);
    assert_ptr_equal(mpc, (void *)mpc_store.raw);

    /* 4. Test when SOC is above target (0.80 > 0.60) -> should discharge (optimal power > 0 or
     * negative sign depending on convention) */
    double opt_p = 0.0;
    assert_int_equal(mpc_dispatcher_step(mpc, 0.80, &opt_p), 0);
    assert_true(opt_p != 0.0);
    assert_int_equal(mpc_dispatcher_get_step_count(mpc), 1);

    /* 5. Test step via ports */
    uint32_t tag_soc = 401;
    uint32_t tag_p_out = 402;
    mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_soc, 75.0, 1000ULL); /* 75% */

    assert_int_equal(mpc_dispatcher_step_ports(mpc, tag_soc, tag_p_out), 0);

    double read_p = 0.0;
    mock_ports.reader.read_tag(mock_ports.reader.ctx, tag_p_out, &read_p, NULL);
    assert_true(read_p != 0.0);
}

static void test_composition_root_caller_allocated_storage(void **state) {
    (void)state;

    /* 1. Allocate Composition Root Container storage on stack (0 malloc) */
    edge_composition_container_storage_t root_store;
    memset(&root_store, 0x00, sizeof(root_store));

    edge_composition_config_t cfg = {.product_name = "IEMS-BESS-Container",
                                     .datahub_capacity = 256,
                                     .memory_pool_buffer_size = 2048,
                                     .memory_pool_capacity = 4,
                                     .enable_dma_buf = true,
                                     .enable_anti_corruption = true};

    /* 2. In-place placement construct Single Composition Root */
    edge_composition_container_t *root = edge_composition_root_create_in(&root_store, &cfg);
    assert_non_null(root);
    assert_ptr_equal(root, (void *)root_store.raw);

    /* 3. Retrieve injected ports bundle */
    edge_consumer_ports_t ports;
    assert_int_equal(edge_composition_root_get_ports(root, &ports), 0);
    assert_non_null(ports.reader.read_tag);
    assert_non_null(ports.writer.write_tag);
    assert_non_null(ports.clock.now_us);

    /* 4. Telemetry write and read through DataHub shadow */
    uint32_t test_tag = 50;
    assert_int_equal(ports.writer.write_tag(ports.writer.ctx, test_tag, 380.5, 2000000ULL), 0);

    double val = 0.0;
    uint64_t ts = 0;
    assert_int_equal(ports.reader.read_tag(ports.reader.ctx, test_tag, &val, &ts), 0);
    assert_float_equal(val, 380.5, 0.001);

    /* 5. Verify Metrics */
    edge_composition_metrics_t metrics;
    assert_int_equal(edge_composition_root_get_metrics(root, &metrics), 0);
    assert_string_equal(metrics.product_name, "IEMS-BESS-Container");
    assert_true(metrics.initialized);
    assert_true(metrics.zero_alloc_runtime);

    /* 6. Clean symmetric destruction */
    assert_int_equal(edge_composition_root_destroy(root), 0);
}

static void test_micro_isc_caller_allocated_storage(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    memset(&store, 0xEE, sizeof(store));

    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 64, NULL);
    assert_non_null(det);
    assert_ptr_equal(det, (void *)store.raw);

    assert_int_equal(battery_micro_isc_get_cell_count(det), 64);
    assert_float_equal(battery_micro_isc_get_pack_avg_voltage(det), 0.0f, 0.001f);

    /* Feed cell */
    for (uint16_t c = 0; c < 64; c++) {
        assert_int_equal(battery_micro_isc_feed_cell(det, c, 3.300f, 0.0f, 25.0f, 0.50f, 0.0), 0);
    }
}

static void test_battery_balancer_caller_allocated_storage(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    memset(&store, 0xDD, sizeof(store));

    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);
    assert_ptr_equal(bal, (void *)store.raw);

    assert_int_equal(battery_balancer_get_cell_count(bal), 16);
    const battery_bal_config_t *cfg = battery_balancer_get_config(bal);
    assert_non_null(cfg);
    assert_int_equal(cfg->mode, BATTERY_BAL_MODE_ACTIVE_PAIRWISE);

    for (uint16_t i = 0; i < 16; i++) {
        battery_bal_cell_input_t in = {.cell_id = i, .voltage_v = 3.300, .soc = 0.50, .temperature_c = 25.0, .soh = 1.0, .r0_mohm = 0.5};
        assert_int_equal(battery_balancer_update_cell(bal, i, &in), 0);
    }
    assert_int_equal(battery_balancer_plan_step(bal, 0.0), 0);
    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_true(plan->valid);
}

static void test_gas_early_warning_caller_allocated_storage(void **state) {
    (void)state;
    battery_gas_ew_storage_t store;
    memset(&store, 0xBB, sizeof(store));

    battery_gas_ew_detector_t *ew = battery_gas_ew_create_in(&store, NULL, NULL);
    assert_non_null(ew);
    assert_ptr_equal(ew, (void *)store.raw);

    assert_int_equal(battery_gas_ew_get_sample_count(ew), 0);

    battery_gas_sensor_sample_t s = {
        .pressure_pa = 101325.0,
        .max_cell_temp_c = 25.0,
        .co_ppm = 0.0,
        .h2_ppm = 0.0,
        .voc_ppb = 10.0,
        .smoke_density = 0.0,
        .min_cell_volt_v = 3.300,
        .timestamp_s = 100.0
    };
    assert_int_equal(battery_gas_ew_push_sample(ew, &s), 0);
    assert_int_equal(battery_gas_ew_get_sample_count(ew), 1);
    assert_int_equal(battery_gas_ew_process_step(ew, 100.0), 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_NORMAL);
}

static void test_battery_soh_caller_allocated_storage(void **state) {
    (void)state;
    battery_soh_storage_t store;
    memset(&store, 0xAA, sizeof(store));

    battery_soh_estimator_t *est = battery_soh_create_in(&store, NULL);
    assert_non_null(est);
    assert_ptr_equal(est, (void *)store.raw);

    assert_int_equal(battery_soh_feed_sample(est, 0.0, 3.295, 25.0, 0.50, 0.0), 0);
    assert_int_equal(battery_soh_process_fusion(est, 1.0), 0);

    const battery_soh_result_t *res = battery_soh_get_result(est);
    assert_non_null(res);
    assert_true(res->valid);
    assert_true(res->soh_capacity >= 0.98);
}

static void test_battery_eis_caller_allocated_storage(void **state) {
    (void)state;
    battery_eis_storage_t store;
    memset(&store, 0x99, sizeof(store));

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);
    assert_ptr_equal(eis, (void *)store.raw);

    assert_false(battery_eis_is_ready(eis));
    double pert = battery_eis_next_prbs_excitation(eis);
    assert_true(fabs(fabs(pert) - 1.0) < 1e-6);
}

static void test_battery_pulse_heating_caller_allocated_storage(void **state) {
    (void)state;
    pulse_heat_engine_storage_t store;
    memset(&store, 0x88, sizeof(store));

    pulse_heat_engine_t *engine = pulse_heat_engine_create_in(&store, NULL);
    assert_non_null(engine);
    assert_ptr_equal(engine, (void *)store.raw);

    assert_int_equal(pulse_heat_engine_get_state(engine), PULSE_HEAT_STATE_IDLE);
    assert_int_equal(pulse_heat_engine_get_fault(engine), PULSE_HEAT_FAULT_NONE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_battery_ekf_caller_allocated_storage),
        cmocka_unit_test(test_vsg_controller_caller_allocated_storage),
        cmocka_unit_test(test_mpc_dispatcher_caller_allocated_storage),
        cmocka_unit_test(test_composition_root_caller_allocated_storage),
        cmocka_unit_test(test_micro_isc_caller_allocated_storage),
        cmocka_unit_test(test_battery_balancer_caller_allocated_storage),
        cmocka_unit_test(test_gas_early_warning_caller_allocated_storage),
        cmocka_unit_test(test_battery_soh_caller_allocated_storage),
        cmocka_unit_test(test_battery_eis_caller_allocated_storage),
        cmocka_unit_test(test_battery_pulse_heating_caller_allocated_storage),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
