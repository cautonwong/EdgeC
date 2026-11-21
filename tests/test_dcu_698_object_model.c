/**
 * @file test_dcu_698_object_model.c
 * @brief D1-03：静态 698 对象表——查表、mask 与分发。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

typedef struct {
    uint16_t oi;
    uint8_t class_id;
    dcu_obj_scope_t scope;
    uint16_t attr_mask;
    uint16_t method_mask;
} expected_object_t;

/* 与 D1-03 迁移前 dcu_router_init_ex 注册的 12 个 OI 一致。 */
static const expected_object_t k_expected[] = {
    {OI_METER_ARCHIVE, IC_ACQUISITION, DCU_OBJ_SHARED, 0x0004, 0x003E},
    {OI_TASK_SCHEDULER, IC_ACQUISITION, DCU_OBJ_SHARED, 0x0004, 0x0002},
    {(uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, 0x0004, 0x0000},
    {(uint16_t)((OAD_CURRENT >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, 0x0004, 0x0000},
    {(uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), IC_RECORD, DCU_OBJ_SHARED, 0x0004, 0x0000},
    {OI_REMOTE_CONTROL, IC_CONTROL, DCU_OBJ_SINGLETON, 0x0000, 0x0002},
    {OI_ESAM_SECURITY, IC_ACQUISITION, DCU_OBJ_SINGLETON, 0x0000, 0x0002},
    {OI_CHANNEL_CONFIG, IC_ACQUISITION, DCU_OBJ_SINGLETON, 0x000C, 0x0000},
    {OI_METER_DISCOVERY, IC_ACQUISITION, DCU_OBJ_SINGLETON, 0x000C, 0x0000},
    {OI_DISCOVERY_RESULT, IC_ACQUISITION, DCU_OBJ_SINGLETON, 0x0004, 0x0000},
    {OI_VOLTAGE_DROP, IC_EVENT, DCU_OBJ_SHARED, 0x077C, 0x0002},
    {OI_OVER_CURRENT, IC_EVENT, DCU_OBJ_SHARED, 0x077C, 0x0002},
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
        assert_int_equal(obj->attr_mask, k_expected[i].attr_mask);
        assert_int_equal(obj->method_mask, k_expected[i].method_mask);
    }

    /* 缺失 OI 必须查不到。 */
    assert_null(dcu_698_find(0x1234));
    assert_int_equal(dcu_698_read(MK_OAD(0x1234, 2), &(uint16_t){0}), -1);
    assert_int_equal(dcu_698_call(MK_OMD(0x1234, 1), NULL, NULL), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_mask_rejects_missing_attr_and_method(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 6000 只支持属性 2；属性 3 在 mask 外，新的查表 API 直接拒绝。 */
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 3), &(uint16_t){0}), -1);

    /* 6000 方法 6 不存在 -> -1；方法 2 存在但拒绝执行 -> 处理器的 -4。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 6), NULL, NULL), -1);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 2), NULL, NULL), -4);

    /* 6002 只保留遗留方法 127，不属于 698 方法集，mask 不置位。 */
    uint32_t duration = 5;
    int action_res = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_DISCOVERY, 127), &duration, &action_res), -1);

    /* 支持的属性仍然可达。 */
    uint16_t count = 99;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &count), 0);
    assert_int_equal(count, 0);

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

    /* F200 读写：属性 2 = 波特率。 */
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
        cmocka_unit_test_setup_teardown(test_mask_rejects_missing_attr_and_method, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dispatch_read_write_call, setup, teardown),
        cmocka_unit_test_setup_teardown(test_reset_returns_to_uninitialized, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
