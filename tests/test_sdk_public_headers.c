/**
 * @file test_sdk_public_headers.c
 * @brief AEP-401: Unit test verifying secondary developer application using ONLY public SDK header <edgeos/edgeos.h>
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include <uv.h>

// STRICT: Secondary Developer Application ONLY includes public SDK entry point
#include <edgeos/edgeos.h>

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

static int sdk_test_map_fn(uint32_t tag_id) {
    return (int)(tag_id % 512);
}

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_sdk_version_macros(void **state) {
    (void)state;

    // Verify Version Constants
    assert_int_equal(EDGEOS_SDK_VERSION_MAJOR, 5);
    assert_int_equal(EDGEOS_SDK_VERSION_MINOR, 0);
    assert_int_equal(EDGEOS_SDK_VERSION_PATCH, 0);
    assert_string_equal(EDGEOS_SDK_VERSION_STRING, "5.0.0");

    // Verify SDK Initialization & Version Query Function
    assert_int_equal(edgeos_sdk_init(), 0);
    assert_string_equal(edgeos_sdk_get_version(), "5.0.0");
}

static void test_sdk_datahub_usage(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    edgeos_datahub_t hub;
    int rc = datahub_init(&hub, &loop, "sdk_test_hub", 512, 512, NULL, 0, NULL, sdk_test_map_fn, NULL);
    assert_int_equal(rc, 0);

    // Test DataHub shadow write and read using SDK types
    edgeos_pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 220.5, 1000);
    assert_int_equal(datahub_write_shadow(&hub, &msg), 0);

    edgeos_pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(&hub, 1001, &out), 0);
    assert_float_equal(out.payload.val.f, 220.5f, 0.001f);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);
}

static void test_sdk_driver_interface(void **state) {
    (void)state;

    // Simulate 2nd-party custom driver definition using SDK interfaces
    edgeos_driver_ctx_t dummy_ctx;
    memset(&dummy_ctx, 0, sizeof(dummy_ctx));
    dummy_ctx.device_id = 42;
    dummy_ctx.role = PROTOCOL_ROLE_MASTER;
    dummy_ctx.flags = 0x01;

    assert_int_equal(dummy_ctx.device_id, 42);
    assert_int_equal(dummy_ctx.role, PROTOCOL_ROLE_MASTER);
    assert_int_equal(dummy_ctx.flags, 0x01);

    edgeos_point_config_t pt = {
        .tag_id = 1001,
        .reg_addr = 0x3001,
        .reg_count = 1,
        .func_code = 3,
        .scaler = 0.1,
        .offset = 0.0
    };
    assert_int_equal(pt.tag_id, 1001);
    assert_int_equal(pt.reg_addr, 0x3001);
}

static void test_sdk_anti_corruption_usage(void **state) {
    (void)state;

    edgeos_acl_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 16), 0);

    edgeos_acl_entry_t entry = {
        .tag_id = 1001,
        .domain_name = "core_standard",
        .entity_prop = "grid.voltage_phase_a",
        .proto = EDGE_PROTO_MODBUS,
        .raw_address = 0x3001,
        .scaler = 0.1,
        .offset = 0.0,
        .data_type = EDGE_ACL_TYPE_FLOAT
    };
    assert_int_equal(edge_anti_corruption_register(&acl, &entry), 0);

    // Inbound translation via SDK
    edgeos_pipe_msg_t in_msg;
    assert_int_equal(edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x3001, 2300.0, &in_msg), 0);
    assert_float_equal(in_msg.payload.val.f, 230.0f, 0.001f);

    // Outbound translation via SDK
    uint32_t out_addr = 0;
    double out_val = 0.0;
    assert_int_equal(edge_anti_corruption_translate_outbound(&acl, 1001, 230.0, &out_addr, &out_val), 0);
    assert_int_equal(out_addr, 0x3001);
    assert_float_equal((float)out_val, 2300.0f, 0.01f);

    edge_anti_corruption_destroy(&acl);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sdk_version_macros),
        cmocka_unit_test(test_sdk_datahub_usage),
        cmocka_unit_test(test_sdk_driver_interface),
        cmocka_unit_test(test_sdk_anti_corruption_usage),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
