/**
 * @file test_dx_scaffolding.c
 * @brief AEP-304: DX (Developer Experience) Automated Integration Test
 *
 * Verifies that 2nd-party developers can create, declare, register and schedule
 * new drivers with zero SDK source modification, complete VTable dispatch,
 * DataHub telemetry writeback, and lifecycle safety.
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
#include <uv.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"
#include "shmring.h"

/* -------------------------------------------------------------------------- */
/* 1. Simulated 2nd-Party Custom BESS Driver Implementation                   */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t simulated_voltage;
    uint32_t simulated_power;
    bool is_online;
    int request_count;
} custom_bess_ctx_t;

static int custom_bess_on_init(edge_driver_ctx_t *ctx) {
    if (!ctx) return -1;
    custom_bess_ctx_t *priv = (custom_bess_ctx_t *)ctx->priv;
    if (!priv) return -1;
    priv->simulated_voltage = 2300;  // 230.0 V
    priv->simulated_power = 5000;    // 5000 W
    priv->is_online = true;
    priv->request_count = 0;
    return 0;
}

static void custom_bess_on_cleanup(edge_driver_ctx_t *ctx) {
    if (!ctx) return;
    custom_bess_ctx_t *priv = (custom_bess_ctx_t *)ctx->priv;
    if (priv) {
        priv->is_online = false;
    }
}

static bool custom_bess_is_ready(edge_driver_ctx_t *ctx) {
    if (!ctx || !ctx->priv) return false;
    custom_bess_ctx_t *priv = (custom_bess_ctx_t *)ctx->priv;
    return priv->is_online;
}

static int custom_bess_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    if (!ctx) return -1;
    (void)out_tx;
    return 0;
}

static int custom_bess_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids,
                                      int tag_count, net_buf_t *out) {
    if (!ctx || !out || !tag_ids || tag_count <= 0) return -1;
    custom_bess_ctx_t *priv = (custom_bess_ctx_t *)ctx->priv;
    priv->request_count++;

    // Format simulated Modbus/Custom request frame: [Header 0xAA, Count, DeviceID, Tag0...]
    uint8_t header[4] = {0xAA, 0x55, (uint8_t)tag_count, (uint8_t)ctx->device_id};
    net_buf_append_bytes(out, header, sizeof(header));
    return 0;
}

static int custom_bess_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id, double value,
                                    net_buf_t *out) {
    if (!ctx || !out) return -1;
    (void)tag_id;
    (void)value;
    return 0;
}

static int custom_bess_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                       pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    if (!ctx || !in || !out_msgs || !msg_count || !consumed) return -1;
    (void)resp_out;
    custom_bess_ctx_t *priv = (custom_bess_ctx_t *)ctx->priv;

    // Emit 2 decoded telemetry pipe messages
    pipe_msg_init_scalar(&out_msgs[0], 1001, (double)priv->simulated_voltage / 10.0, 1000);
    pipe_msg_init_scalar(&out_msgs[1], 1003, (double)priv->simulated_power, 1000);

    *msg_count = 2;
    *consumed = in->b.len;
    return 0;
}

static int custom_bess_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                 pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    return custom_bess_decode_response(ctx, in, resp_out, async_msgs, async_count, consumed);
}

static const edge_driver_t g_custom_bess_driver = {
    .name = "CUSTOM_BESS",
    .ctx_size = sizeof(custom_bess_ctx_t),
    .on_init = custom_bess_on_init,
    .on_cleanup = custom_bess_on_cleanup,
    .is_ready = custom_bess_is_ready,
    .on_tick = custom_bess_on_tick,
    .encode_request = custom_bess_encode_request,
    .encode_write = custom_bess_encode_write,
    .decode_response = custom_bess_decode_response,
    .handle_rx = custom_bess_handle_rx,
};

/* -------------------------------------------------------------------------- */
/* 2. CMocka Unit Tests                                                      */
/* -------------------------------------------------------------------------- */

static void test_dx_driver_registration_and_lookup(void **state) {
    (void)state;
    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);

    // 1. Declare generated-style driver registry table containing the custom driver
    static const edge_driver_reg_entry_t custom_registry[] = {
        {.protocol_name = "CUSTOM_BESS", .driver = &g_custom_bess_driver},
    };

    // 2. Batch register table into driver manager
    int rc = driver_manager_register_table(mgr, custom_registry, 1);
    assert_int_equal(rc, 0);

    // 3. Lookup custom driver by name
    const edge_driver_t *found = driver_manager_find(mgr, "CUSTOM_BESS");
    assert_ptr_equal(found, &g_custom_bess_driver);
    assert_string_equal(found->name, "CUSTOM_BESS");
    assert_int_equal(found->ctx_size, sizeof(custom_bess_ctx_t));

    driver_manager_destroy(mgr);
}

static void test_dx_device_lifecycle_and_io(void **state) {
    (void)state;
    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);

    static const edge_driver_reg_entry_t custom_registry[] = {
        {.protocol_name = "CUSTOM_BESS", .driver = &g_custom_bess_driver},
    };
    assert_int_equal(driver_manager_register_table(mgr, custom_registry, 1), 0);

    // 1. Create driver instance for device ID 42
    edge_driver_ctx_t *ctx = driver_instance_create(mgr, "CUSTOM_BESS", 42);
    assert_non_null(ctx);
    assert_ptr_equal(ctx->driver, &g_custom_bess_driver);
    assert_int_equal(ctx->device_id, 42);
    assert_non_null(ctx->priv);

    // 2. Verify readiness
    assert_true(g_custom_bess_driver.is_ready(ctx));

    // 3. Test encode_request
    net_buf_pool_t *pool = net_buf_pool_new(8, 256, 0);
    assert_non_null(pool);
    net_buf_t *tx_buf = net_buf_alloc(pool, 0);
    assert_non_null(tx_buf);

    uint32_t tag_ids[2] = {1001, 1003};
    int enc_rc = g_custom_bess_driver.encode_request(ctx, tag_ids, 2, tx_buf);
    assert_int_equal(enc_rc, 0);
    assert_int_equal(tx_buf->b.len, 4);
    assert_int_equal(tx_buf->b.data[0], 0xAA);
    assert_int_equal(tx_buf->b.data[1], 0x55);
    assert_int_equal(tx_buf->b.data[2], 2);
    assert_int_equal(tx_buf->b.data[3], 42);

    // 4. Test decode_response
    net_buf_t *rx_buf = net_buf_alloc(pool, 0);
    assert_non_null(rx_buf);
    uint8_t sim_rx[6] = {0xAA, 0x55, 0x02, 0x00, 0x00, 0x00};
    net_buf_append_bytes(rx_buf, sim_rx, sizeof(sim_rx));

    pipe_msg_t msgs[8];
    int msg_count = 0;
    size_t consumed = 0;
    int dec_rc =
        g_custom_bess_driver.decode_response(ctx, rx_buf, NULL, msgs, &msg_count, &consumed);
    assert_int_equal(dec_rc, 0);
    assert_int_equal(msg_count, 2);
    assert_int_equal(msgs[0].tag_id, 1001);
    assert_float_equal(msgs[0].payload.val.f, 230.0, 0.001);
    assert_int_equal(msgs[1].tag_id, 1003);
    assert_float_equal(msgs[1].payload.val.f, 5000.0, 0.001);

    // 5. Clean up instance and pools
    net_buf_unref(tx_buf);
    net_buf_unref(rx_buf);
    net_buf_pool_destroy(pool);

    driver_instance_destroy(ctx, "CUSTOM_BESS");
    driver_manager_destroy(mgr);
}

static void test_dx_datahub_telemetry_integration(void **state) {
    (void)state;
    // 1. Initialize DataHub instance with libuv loop
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    int dh_rc = datahub_init(&hub, &loop, "dx_shm_hub", 1024, 1024, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(dh_rc, 0);

    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);

    static const edge_driver_reg_entry_t custom_registry[] = {
        {.protocol_name = "CUSTOM_BESS", .driver = &g_custom_bess_driver},
    };
    assert_int_equal(driver_manager_register_table(mgr, custom_registry, 1), 0);

    edge_driver_ctx_t *ctx = driver_instance_create(mgr, "CUSTOM_BESS", 101);
    assert_non_null(ctx);
    ctx->user_data = &hub;  // Dependency injection of DataHub

    // 2. Simulate decode & writeback loop
    net_buf_pool_t *pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    net_buf_t *rx_buf = net_buf_alloc(pool, 0);
    assert_non_null(rx_buf);
    uint8_t sim_rx[4] = {0xAA, 0x55, 0x01, 0x00};
    net_buf_append_bytes(rx_buf, sim_rx, sizeof(sim_rx));

    pipe_msg_t msgs[4];
    int msg_count = 0;
    size_t consumed = 0;
    assert_int_equal(
        g_custom_bess_driver.decode_response(ctx, rx_buf, NULL, msgs, &msg_count, &consumed), 0);

    // 3. Dispatch decoded points into DataHub shadow table
    for (int i = 0; i < msg_count; i++) {
        int w_rc = datahub_write_shadow(&hub, &msgs[i]);
        assert_int_equal(w_rc, 0);
    }

    // 4. Read back and verify physical truth in DataHub shadow table
    pipe_msg_t read_msg1, read_msg2;
    assert_int_equal(datahub_read_shadow(&hub, 1001, &read_msg1), 0);
    assert_float_equal(read_msg1.payload.val.f, 230.0, 0.001);

    assert_int_equal(datahub_read_shadow(&hub, 1003, &read_msg2), 0);
    assert_float_equal(read_msg2.payload.val.f, 5000.0, 0.001);

    // 5. Clean up
    net_buf_unref(rx_buf);
    net_buf_pool_destroy(pool);
    driver_instance_destroy(ctx, "CUSTOM_BESS");
    driver_manager_destroy(mgr);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

static void test_dx_error_handling_and_edge_cases(void **state) {
    (void)state;
    driver_manager_t *mgr = driver_manager_create();
    assert_non_null(mgr);

    // 1. Lookup non-existent driver
    assert_null(driver_manager_find(mgr, "NON_EXISTENT"));
    assert_null(driver_instance_create(mgr, "NON_EXISTENT", 1));

    // 2. NULL inputs to register table
    assert_int_equal(driver_manager_register_table(NULL, NULL, 0), -1);
    assert_int_equal(driver_manager_register_table(mgr, NULL, 5), -1);

    // 3. Table with NULL entry driver pointer
    static const edge_driver_reg_entry_t bad_registry[] = {
        {.protocol_name = "NULL_DRV", .driver = NULL},
    };
    assert_int_equal(driver_manager_register_table(mgr, bad_registry, 1), -1);

    // 4. Safe destroy on NULL instance
    driver_instance_destroy(NULL, "CUSTOM_BESS");

    driver_manager_destroy(mgr);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dx_driver_registration_and_lookup),
        cmocka_unit_test(test_dx_device_lifecycle_and_io),
        cmocka_unit_test(test_dx_datahub_telemetry_integration),
        cmocka_unit_test(test_dx_error_handling_and_edge_cases),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
