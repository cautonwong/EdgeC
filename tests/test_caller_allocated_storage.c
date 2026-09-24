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

#include "battery_fo_ecm_ekf.h"
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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_battery_ekf_caller_allocated_storage),
        cmocka_unit_test(test_vsg_controller_caller_allocated_storage),
        cmocka_unit_test(test_mpc_dispatcher_caller_allocated_storage),
        cmocka_unit_test(test_composition_root_caller_allocated_storage),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
