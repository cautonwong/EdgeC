/**
 * @file test_dcu_698_object_model.c
 * @brief 静态 698 对象表：查表、256 位 mask 与分发。
 */

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
static const uint8_t k_attr_event[] = {2, 3, 4, 5, 6, 8, 9, 10};
static const uint8_t k_method_1[] = {1};
static const uint8_t k_method_127[] = {127};
static const uint8_t k_method_6000[] = {1, 2, 3, 4, 5, 127, 128};
static const uint8_t k_method_5004[] = {3, 4, 5, 7, 8};

/* 12 个 OI 及其 mask 必须与处理器实现保持一致。 */
static const expected_object_t k_expected[] = {
    {OI_METER_ARCHIVE, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2, 1, k_method_6000, 7},
    {OI_TASK_SCHEDULER, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2, 1, k_method_1, 1},
    {(uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_2, 1, NULL,
     0},
    {(uint16_t)((OAD_CURRENT >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_2, 1, NULL,
     0},
    {(uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), IC_RECORD, DCU_OBJ_SHARED, k_attr_2, 1,
     k_method_5004, 5},
    {OI_REMOTE_CONTROL, IC_CONTROL, DCU_OBJ_SINGLETON, NULL, 0, k_method_1, 1},
    {OI_ESAM_SECURITY, IC_ACQUISITION, DCU_OBJ_SINGLETON, NULL, 0, k_method_1, 1},
    {OI_CHANNEL_CONFIG, IC_ACQUISITION, DCU_OBJ_SINGLETON, k_attr_23, 2, NULL, 0},
    {OI_METER_DISCOVERY, IC_ACQUISITION, DCU_OBJ_SINGLETON, k_attr_23, 2, k_method_127, 1},
    {OI_DISCOVERY_RESULT, IC_ACQUISITION, DCU_OBJ_SINGLETON, k_attr_2, 1, NULL, 0},
    {OI_VOLTAGE_DROP, IC_EVENT, DCU_OBJ_SHARED, k_attr_event, 8, k_method_1, 1},
    {OI_OVER_CURRENT, IC_EVENT, DCU_OBJ_SHARED, k_attr_event, 8, k_method_1, 1},
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

        /* 宽 id 的边界：属性 18 与不存在的属性 200 都不得被误置位。 */
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

    /* 6000 方法 6 不存在 -> -1；方法 2 存在但拒绝执行 -> 处理器的 -4。 */
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

    /* 129 未实现，不得因“127/128 可用”被误置位。 */
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

static void test_facade_and_table_api_agree(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 同一 OMD：门面与表驱动 API 必须给同一个返回码。 */
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
        cmocka_unit_test_setup_teardown(test_facade_and_table_api_agree, setup, teardown),
        cmocka_unit_test_setup_teardown(test_reset_returns_to_uninitialized, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
