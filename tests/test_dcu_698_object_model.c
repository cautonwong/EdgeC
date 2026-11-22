#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_object_model.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

typedef struct {
    uint16_t oi;
    uint8_t class_id;
    dcu_obj_scope_t scope;
    const uint8_t *attrs;
    size_t attr_count;
    const uint8_t *methods;
    size_t method_count;
} expected_object_t;

static const uint8_t k_attr_2[] = {2};
static const uint8_t k_attr_23[] = {2, 3};
static const uint8_t k_attr_event[] = {4, 11, 12};
static const uint8_t k_method_1[] = {1};
static const uint8_t k_method_127[] = {127};
static const uint8_t k_method_8000[] = {1, 127, 128, 129, 130, 131};
static const uint8_t k_method_6000[] = {1, 2, 3, 4, 5, 127, 128};
static const uint8_t k_method_5004[] = {3, 4, 5, 7, 8};

static const expected_object_t k_expected[] = {
    {OI_METER_ARCHIVE, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2, 1, k_method_6000, 7},
    {OI_TASK_SCHEDULER, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2, 1, k_method_1, 1},
    {(uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_23, 2,
     NULL, 0},
    {(uint16_t)((OAD_CURRENT >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_23, 2, NULL,
     0},
    {(uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), IC_RECORD, DCU_OBJ_SHARED, k_attr_2, 1,
     k_method_5004, 5},
    {OI_REMOTE_CONTROL, IC_CONTROL, DCU_OBJ_SINGLETON, NULL, 0, k_method_8000, 6},
    {OI_ESAM_SECURITY, IC_ESAM, DCU_OBJ_SINGLETON, NULL, 0, k_method_1, 1},
    {OI_CHANNEL_CONFIG, IC_PERIPHERAL, DCU_OBJ_SINGLETON, k_attr_23, 2, NULL, 0},
    {OI_METER_DISCOVERY, IC_ACQUISITION, DCU_OBJ_SINGLETON, k_attr_23, 2, k_method_127, 1},
    {OI_DISCOVERY_RESULT, IC_ACQUISITION, DCU_OBJ_SINGLETON, k_attr_2, 1, NULL, 0},
    {OI_VOLTAGE_DROP, IC_SUB_EVENT, DCU_OBJ_SHARED, k_attr_event, 3, k_method_1, 1},
    {OI_OVER_CURRENT, IC_SUB_EVENT, DCU_OBJ_SHARED, k_attr_event, 3, k_method_1, 1},
};

static int setup(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    return 0;
}

static int teardown(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    return 0;
}

static void init_model(dcu_inventory_t *inv, dcu_poller_t *poller) {
    assert_int_equal(dcu_inventory_init(inv), 0);
    assert_int_equal(dcu_poller_init(poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(inv, poller, sink, NULL, NULL, NULL, NULL, bus), 0);
}

static void assert_mask_matches(const uint8_t actual[32], const uint8_t *ids, size_t count) {
    uint8_t want[32] = {0};
    for (size_t i = 0; i < count; i++) dcu_mask_set(want, ids[i]);
    assert_memory_equal(actual, want, sizeof(want));
}

static void test_all_ois_resolvable_with_class_and_mask(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    for (size_t i = 0; i < sizeof(k_expected) / sizeof(k_expected[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(k_expected[i].oi);
        assert_non_null(obj);
        assert_int_equal(obj->oi, k_expected[i].oi);
        assert_int_equal(obj->class_id, k_expected[i].class_id);
        assert_int_equal(obj->scope, k_expected[i].scope);
        assert_mask_matches(obj->attr_mask, k_expected[i].attrs, k_expected[i].attr_count);
        assert_mask_matches(obj->method_mask, k_expected[i].methods, k_expected[i].method_count);

        assert_false(dcu_mask_test(obj->attr_mask, 18));
        assert_false(dcu_mask_test(obj->attr_mask, 200));
    }

    assert_null(dcu_698_find(0x1234));
    assert_int_equal(dcu_698_read(MK_OAD(0x1234, 2), &(uint16_t){0}), -1);
    assert_int_equal(dcu_698_call(MK_OMD(0x1234, 1), NULL, NULL), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_mask_bitmap_supports_wide_ids(void **state) {
    (void)state;
    uint8_t mask[32] = {0};

    dcu_mask_set(mask, 18);
    assert_true(dcu_mask_test(mask, 18));
    assert_false(dcu_mask_test(mask, 17));
    assert_false(dcu_mask_test(mask, 19));

    dcu_mask_set(mask, 200);
    assert_true(dcu_mask_test(mask, 200));
    assert_false(dcu_mask_test(mask, 199));
    assert_false(dcu_mask_test(mask, 201));
}

static void test_mask_rejects_missing_attr_and_method(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 3), &(uint16_t){0}), -1);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 18), &(uint16_t){0}), -1);

    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 6), NULL, NULL), -1);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 2), NULL, NULL), -4);

    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 200), NULL, NULL), -1);

    uint16_t count = 99;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &count), 0);
    assert_int_equal(count, 0);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_legacy_methods_127_and_128_dispatch(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    int method_res = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 127), "20000001", &method_res), 0);
    assert_int_equal(method_res, 0);

    uint16_t count = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &count), 0);
    assert_int_equal(count, 1);

    method_res = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 128), "20000001", &method_res), 0);
    assert_int_equal(method_res, 0);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &count), 0);
    assert_int_equal(count, 0);

    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 129), NULL, NULL), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_dispatch_read_write_call(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    int method_res = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 1), "20000001", &method_res), 0);
    assert_int_equal(method_res, 0);

    uint16_t count = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &count), 0);
    assert_int_equal(count, 1);

    uint32_t baud = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_CHANNEL_CONFIG, 2), &baud), 0);
    assert_int_equal(baud, 9600);
    uint32_t new_baud = 2400;
    assert_int_equal(dcu_698_write(MK_OAD(OI_CHANNEL_CONFIG, 2), &new_baud), 0);
    baud = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_CHANNEL_CONFIG, 2), &baud), 0);
    assert_int_equal(baud, 2400);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void init_model_with_events(dcu_inventory_t *inv, dcu_poller_t *poller,
                                   dcu_event_obj_t *evt_3000, dcu_event_obj_t *evt_3005) {
    assert_int_equal(dcu_inventory_init(inv), 0);
    assert_int_equal(dcu_poller_init(poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(inv, poller, sink, NULL, NULL, evt_3000, evt_3005, bus), 0);
}

static void test_event_class24_attr_reads(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_event_obj_t evt_3000 = {.oi = OI_VOLTAGE_DROP, .report_flag = 2, .is_enabled = true};
    init_model_with_events(&inv, &poller, &evt_3000, NULL);

    uint16_t max_records = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 4), &max_records), 0);
    assert_int_equal(max_records, MAX_EVENT_RECORDS);

    uint8_t report_flag = 0xFF;
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 11), &report_flag), 0);
    assert_int_equal(report_flag, 2);

    bool is_enabled = false;
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 12), &is_enabled), 0);
    assert_true(is_enabled);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_event_class24_attr_writes(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_event_obj_t evt_3005 = {.oi = OI_OVER_CURRENT};
    init_model_with_events(&inv, &poller, NULL, &evt_3005);

    uint8_t report_flag = 3;
    assert_int_equal(dcu_698_write(MK_OAD(OI_OVER_CURRENT, 11), &report_flag), 0);
    assert_int_equal(evt_3005.report_flag, 3);

    report_flag = 4;
    assert_int_equal(dcu_698_write(MK_OAD(OI_OVER_CURRENT, 11), &report_flag), -2);
    assert_int_equal(evt_3005.report_flag, 3);

    bool is_enabled = true;
    assert_int_equal(dcu_698_write(MK_OAD(OI_OVER_CURRENT, 12), &is_enabled), 0);
    assert_true(evt_3005.is_enabled);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_event_class7_attr_numbers_are_not_served(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_event_obj_t evt_3000 = {.oi = OI_VOLTAGE_DROP};
    dcu_event_obj_t evt_3005 = {.oi = OI_OVER_CURRENT};
    init_model_with_events(&inv, &poller, &evt_3000, &evt_3005);

    static const uint16_t ois[] = {OI_VOLTAGE_DROP, OI_OVER_CURRENT};
    static const uint8_t removed[] = {2, 3, 5, 6, 7, 8, 9, 10, 13, 14};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        for (size_t k = 0; k < sizeof(removed) / sizeof(removed[0]); k++) {
            uint64_t buf = 0;
            assert_int_equal(dcu_698_read(MK_OAD(ois[i], removed[k]), &buf), -1);
            assert_int_equal(dcu_698_write(MK_OAD(ois[i], removed[k]), &buf), -1);
        }
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_event_method1_resets_records(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_event_obj_t evt_3000 = {.oi = OI_VOLTAGE_DROP,
                                .record_count = 2,
                                .last_start_time = 111,
                                .last_end_time = 222};
    evt_3000.records[0].is_active = true;
    init_model_with_events(&inv, &poller, &evt_3000, NULL);

    int result = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 1), NULL, &result), 0);
    assert_int_equal(result, 0);
    assert_int_equal(evt_3000.record_count, 0);
    assert_false(evt_3000.records[0].is_active);
    assert_int_equal(evt_3000.last_start_time, 0);
    assert_int_equal(evt_3000.last_end_time, 0);

    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 2), NULL, NULL), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_facade_and_table_api_agree(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    assert_int_equal(dcu_router_action(MK_OMD(OI_METER_ARCHIVE, 2), NULL, NULL),
                     dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 2), NULL, NULL));
    assert_int_equal(dcu_router_action(MK_OMD(OI_METER_ARCHIVE, 200), NULL, NULL),
                     dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 200), NULL, NULL));
    assert_int_equal(dcu_router_action(MK_OMD(OI_REMOTE_CONTROL, 1), NULL, NULL),
                     dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 1), NULL, NULL));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_reset_returns_to_uninitialized(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);
    assert_non_null(dcu_698_find(OI_METER_ARCHIVE));

    dcu_698_object_model_reset();
    assert_null(dcu_698_find(OI_METER_ARCHIVE));
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &(uint16_t){0}), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_all_ois_resolvable_with_class_and_mask, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_mask_bitmap_supports_wide_ids, setup, teardown),
        cmocka_unit_test_setup_teardown(test_mask_rejects_missing_attr_and_method, setup, teardown),
        cmocka_unit_test_setup_teardown(test_legacy_methods_127_and_128_dispatch, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dispatch_read_write_call, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class24_attr_reads, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class24_attr_writes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class7_attr_numbers_are_not_served, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_event_method1_resets_records, setup, teardown),
        cmocka_unit_test_setup_teardown(test_facade_and_table_api_agree, setup, teardown),
        cmocka_unit_test_setup_teardown(test_reset_returns_to_uninitialized, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
