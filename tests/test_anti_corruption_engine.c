/**
 * @file test_anti_corruption_engine.c
 * @brief AEP-353: Unit tests for Anti-Corruption Layer (ACL) translation engine
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

#include "edge_anti_corruption.h"

/* -------------------------------------------------------------------------- */
/* Test Point Definitions                                                     */
/* -------------------------------------------------------------------------- */

static const edge_anti_corruption_entry_t g_test_acl_table[] = {
    {.tag_id = 1001,
     .domain_name = "core_standard",
     .entity_prop = "grid.voltage_phase_a",
     .proto = EDGE_PROTO_MODBUS,
     .raw_address = 0x3001,
     .scaler = 0.1,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_FLOAT},
    {.tag_id = 1004,
     .domain_name = "core_standard",
     .entity_prop = "grid.current_phase_a",
     .proto = EDGE_PROTO_DLT645,
     .raw_address = 0x00010000,
     .scaler = 0.01,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_FLOAT},
    {.tag_id = 65538,  // 0x010002
     .domain_name = "dcu_distribution",
     .entity_prop = "branch.lineloss_pct",
     .proto = EDGE_PROTO_MODBUS,
     .raw_address = 0x4001,
     .scaler = 0.01,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_FLOAT},
    {.tag_id = 65541,  // 0x010005
     .domain_name = "dcu_distribution",
     .entity_prop = "telecontrol.do_bitmap",
     .proto = EDGE_PROTO_MODBUS,
     .raw_address = 0x4005,
     .scaler = 1.0,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_UINT32},
    {.tag_id = 131074,  // 0x020002
     .domain_name = "iems_microgrid",
     .entity_prop = "pcs.active_power_setpoint",
     .proto = EDGE_PROTO_MODBUS,
     .raw_address = 0x5001,
     .scaler = 0.1,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_FLOAT},
    {.tag_id = 196609,  // 0x030001
     .domain_name = "smart_box_panel",
     .entity_prop = "breaker.residual_current_ma",
     .proto = EDGE_PROTO_MODBUS,
     .raw_address = 0x6001,
     .scaler = 0.1,
     .offset = 0.0,
     .data_type = EDGE_ACL_TYPE_FLOAT}};
static const size_t g_test_acl_count = sizeof(g_test_acl_table) / sizeof(g_test_acl_table[0]);

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_acl_init_and_registration(void **state) {
    (void)state;

    edge_anti_corruption_engine_t acl;
    // Initialize with small capacity to test dynamic expansion
    assert_int_equal(edge_anti_corruption_init(&acl, 2), 0);

    // Register test table
    assert_int_equal(edge_anti_corruption_register_table(&acl, g_test_acl_table, g_test_acl_count),
                     0);
    assert_int_equal(acl.count, g_test_acl_count);
    assert_true(acl.capacity >= g_test_acl_count);

    // Lookup by Tag
    const edge_anti_corruption_entry_t *entry = edge_anti_corruption_lookup_by_tag(&acl, 1001);
    assert_non_null(entry);
    assert_string_equal(entry->entity_prop, "grid.voltage_phase_a");
    assert_int_equal(entry->raw_address, 0x3001);

    // Lookup by Address
    entry = edge_anti_corruption_lookup_by_address(&acl, EDGE_PROTO_DLT645, 0x00010000);
    assert_non_null(entry);
    assert_int_equal(entry->tag_id, 1004);
    assert_string_equal(entry->entity_prop, "grid.current_phase_a");

    // NULL lookups
    assert_null(edge_anti_corruption_lookup_by_tag(&acl, 999999));
    assert_null(edge_anti_corruption_lookup_by_address(&acl, EDGE_PROTO_CAN, 0x9999));

    // Destroy
    edge_anti_corruption_destroy(&acl);
    assert_null(acl.entries);
    assert_false(acl.initialized);
}

static void test_acl_inbound_translation(void **state) {
    (void)state;

    edge_anti_corruption_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 16), 0);
    assert_int_equal(edge_anti_corruption_register_table(&acl, g_test_acl_table, g_test_acl_count),
                     0);

    // 1. Modbus Voltage Inbound: raw 2305 * 0.1 -> 230.5 V
    pipe_msg_t msg;
    int rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x3001, 2305.0, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal(msg.tag_id, 1001);
    assert_float_equal(msg.payload.val.f, 230.5f, 0.001f);

    // 2. DLT645 Current Inbound: raw 1250 * 0.01 -> 12.5 A
    rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_DLT645, 0x00010000, 1250.0, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal(msg.tag_id, 1004);
    assert_float_equal(msg.payload.val.f, 12.5f, 0.001f);

    // 3. DCU Line Loss Inbound: raw 345 * 0.01 -> 3.45 %
    rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x4001, 345.0, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal(msg.tag_id, 65538);
    assert_float_equal(msg.payload.val.f, 3.45f, 0.001f);

    // 4. IEMS Active Power Inbound: raw 50000 * 0.1 -> 5000.0 kW
    rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x5001, 50000.0, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal(msg.tag_id, 131074);
    assert_float_equal(msg.payload.val.f, 5000.0f, 0.001f);

    // 5. SmartBox Residual Current: raw 148 * 0.1 -> 14.8 mA
    rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x6001, 148.0, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal(msg.tag_id, 196609);
    assert_float_equal(msg.payload.val.f, 14.8f, 0.001f);

    // 6. Unknown address inbound failure
    rc = edge_anti_corruption_translate_inbound(&acl, EDGE_PROTO_MODBUS, 0x9999, 100.0, &msg);
    assert_int_equal(rc, -1);

    edge_anti_corruption_destroy(&acl);
}

static void test_acl_outbound_translation(void **state) {
    (void)state;

    edge_anti_corruption_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 16), 0);
    assert_int_equal(edge_anti_corruption_register_table(&acl, g_test_acl_table, g_test_acl_count),
                     0);

    // 1. IEMS Set Active Power (Tag 131074): 5000.0 kW -> raw address 0x5001, raw value 50000.0
    uint32_t raw_addr = 0;
    double raw_val = 0.0;
    int rc = edge_anti_corruption_translate_outbound(&acl, 131074, 5000.0, &raw_addr, &raw_val);
    assert_int_equal(rc, 0);
    assert_int_equal(raw_addr, 0x5001);
    assert_float_equal((float)raw_val, 50000.0f, 0.01f);

    // 2. DCU DO Trip (Tag 65541): 1.0 -> raw address 0x4005, raw value 1.0
    rc = edge_anti_corruption_translate_outbound(&acl, 65541, 1.0, &raw_addr, &raw_val);
    assert_int_equal(rc, 0);
    assert_int_equal(raw_addr, 0x4005);
    assert_float_equal((float)raw_val, 1.0f, 0.01f);

    // 3. Unknown Tag outbound failure
    rc = edge_anti_corruption_translate_outbound(&acl, 999999, 100.0, &raw_addr, &raw_val);
    assert_int_equal(rc, -1);

    edge_anti_corruption_destroy(&acl);
}

static void test_acl_domain_entity_isolation(void **state) {
    (void)state;

    edge_anti_corruption_engine_t acl;
    assert_int_equal(edge_anti_corruption_init(&acl, 16), 0);
    assert_int_equal(edge_anti_corruption_register_table(&acl, g_test_acl_table, g_test_acl_count),
                     0);

    // Verify all registered entries belong to explicit domain names
    for (size_t i = 0; i < acl.count; i++) {
        assert_true(strlen(acl.entries[i].domain_name) > 0);
        assert_true(strlen(acl.entries[i].entity_prop) > 0);
        assert_true(acl.entries[i].tag_id > 0);
    }

    edge_anti_corruption_destroy(&acl);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_acl_init_and_registration),
        cmocka_unit_test(test_acl_inbound_translation),
        cmocka_unit_test(test_acl_outbound_translation),
        cmocka_unit_test(test_acl_domain_entity_isolation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
