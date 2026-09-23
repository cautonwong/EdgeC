/**
 * @file test_composition_root.c
 * @brief Unit Tests for Single Composition Root & Consumer-Defined Ports (AEP-751 / Issue #336)
 */

#include <cmocka.h>
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
#include "edge_port.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_consumer_ports_mock_isolation(void **state) {
    (void)state;

    edge_consumer_ports_t mock_ports;
    assert_int_equal(edge_port_create_mock_ports(&mock_ports, 0.0), 0);

    /* Setup mock battery values */
    uint32_t tag_cur = 101;
    uint32_t tag_volt = 102;
    uint32_t tag_temp = 103;
    uint32_t tag_soc = 104;
    uint32_t tag_ocv = 105;

    assert_int_equal(mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_cur, -10.0, 1000000ULL),
                     0);  // 10A discharge
    assert_int_equal(mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_volt, 3.28, 1000000ULL),
                     0);  // 3.28V
    assert_int_equal(mock_ports.writer.write_tag(mock_ports.writer.ctx, tag_temp, 25.0, 1000000ULL),
                     0);  // 25C

    /* Initialize battery EKF algorithm */
    fo_ecm_ekf_t ekf;
    fo_ecm_params_t params = {.alpha = 0.75,
                              .r0 = 0.005,
                              .rp = 0.008,
                              .c_alpha = 1200.0,
                              .capacity_ah = 100.0,
                              .coulombic_eff = 0.998,
                              .temp_coeff_r0 = 0.002,
                              .temp_ref_c = 25.0};
    assert_true(fo_ecm_ekf_init(&ekf, &params, 0.80, 25.0, 1.0));

    fo_ecm_port_binding_t binding = {.tag_current_a = tag_cur,
                                     .tag_voltage_v = tag_volt,
                                     .tag_temp_c = tag_temp,
                                     .tag_soc_est = tag_soc,
                                     .tag_ocv_est = tag_ocv};

    /* Execute algorithm driven entirely via Consumer-Defined Ports */
    assert_true(fo_ecm_ekf_step_ports(&ekf, &mock_ports.reader, &mock_ports.writer,
                                      &mock_ports.clock, &binding, 1.0));

    /* Verify output written back to port */
    double estimated_soc = 0.0;
    assert_int_equal(
        mock_ports.reader.read_tag(mock_ports.reader.ctx, tag_soc, &estimated_soc, NULL), 0);
    assert_true(estimated_soc > 0.0 && estimated_soc <= 1.0);
}

static void test_composition_root_lifecycle(void **state) {
    (void)state;

    edge_composition_config_t cfg = {.product_name = "IEMS-ESS-Container",
                                     .datahub_capacity = 512,
                                     .memory_pool_buffer_size = 2048,
                                     .memory_pool_capacity = 8,
                                     .enable_dma_buf = true,
                                     .enable_anti_corruption = true};

    edge_composition_container_t *root = NULL;
    assert_int_equal(edge_composition_root_init(&root, &cfg), 0);
    assert_non_null(root);

    datahub_t *hub = edge_composition_root_get_datahub(root);
    assert_non_null(hub);

    edge_consumer_ports_t ports;
    assert_int_equal(edge_composition_root_get_ports(root, &ports), 0);
    assert_non_null(ports.reader.read_tag);
    assert_non_null(ports.writer.write_tag);
    assert_non_null(ports.clock.now_us);
    assert_non_null(ports.memory.acquire_buffer);
    assert_non_null(ports.logger.log_msg);

    edge_composition_metrics_t metrics;
    assert_int_equal(edge_composition_root_get_metrics(root, &metrics), 0);
    assert_string_equal(metrics.product_name, "IEMS-ESS-Container");
    assert_true(metrics.initialized);
    assert_true(metrics.zero_alloc_runtime);

    assert_int_equal(edge_composition_root_destroy(root), 0);
}

typedef struct {
    edge_consumer_ports_t ports;
    uint32_t write_count;
} dummy_consumer_module_t;

static int wire_dummy_consumer(void *ctx, const edge_consumer_ports_t *ports) {
    dummy_consumer_module_t *mod = (dummy_consumer_module_t *)ctx;
    mod->ports = *ports;
    return 0;
}

static void test_composition_root_dependency_injection(void **state) {
    (void)state;

    edge_composition_config_t cfg = {.product_name = "DCU-Smart-Grid-Terminal",
                                     .datahub_capacity = 256,
                                     .memory_pool_buffer_size = 1024,
                                     .memory_pool_capacity = 4,
                                     .enable_dma_buf = true,
                                     .enable_anti_corruption = false};

    edge_composition_container_t *root = NULL;
    assert_int_equal(edge_composition_root_init(&root, &cfg), 0);

    dummy_consumer_module_t mod = {0};
    assert_int_equal(edge_composition_root_inject_consumer(root, &mod, wire_dummy_consumer), 0);

    /* Test port read/write through DataHub shadow */
    uint32_t tag = 201;
    assert_int_equal(mod.ports.writer.write_tag(mod.ports.writer.ctx, tag, 220.5, 5000000ULL), 0);

    double read_val = 0.0;
    uint64_t ts_us = 0;
    assert_int_equal(mod.ports.reader.read_tag(mod.ports.reader.ctx, tag, &read_val, &ts_us), 0);
    assert_float_equal(read_val, 220.5, 0.001);

    edge_composition_metrics_t metrics;
    assert_int_equal(edge_composition_root_get_metrics(root, &metrics), 0);
    assert_int_equal(metrics.active_consumers, 1);

    assert_int_equal(edge_composition_root_destroy(root), 0);
}

static void test_composition_root_zero_allocation_memory_port(void **state) {
    (void)state;

    edge_composition_config_t cfg = {.product_name = "SmartBox-Gateway",
                                     .datahub_capacity = 128,
                                     .memory_pool_buffer_size = 4096,
                                     .memory_pool_capacity = 4,
                                     .enable_dma_buf = true,
                                     .enable_anti_corruption = false};

    edge_composition_container_t *root = NULL;
    assert_int_equal(edge_composition_root_init(&root, &cfg), 0);

    edge_consumer_ports_t ports;
    assert_int_equal(edge_composition_root_get_ports(root, &ports), 0);

    void *buf_ptr = NULL;
    int buf_fd = -1;
    assert_int_equal(ports.memory.acquire_buffer(ports.memory.ctx, 4096, &buf_ptr, &buf_fd), 0);
    assert_non_null(buf_ptr);

    /* Write data without dynamic malloc */
    memset(buf_ptr, 0x5A, 4096);
    assert_int_equal(((uint8_t *)buf_ptr)[0], 0x5A);

    assert_int_equal(ports.memory.release_buffer(ports.memory.ctx, buf_ptr), 0);
    assert_int_equal(edge_composition_root_destroy(root), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_consumer_ports_mock_isolation),
        cmocka_unit_test(test_composition_root_lifecycle),
        cmocka_unit_test(test_composition_root_dependency_injection),
        cmocka_unit_test(test_composition_root_zero_allocation_memory_port),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
