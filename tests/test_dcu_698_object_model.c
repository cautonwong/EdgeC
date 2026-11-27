#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_object_model.h"
#include "dcu_698_catalog_expected.h"
#include "dcu_discovery.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "edge_slave_plane.h"

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

static const uint8_t k_attr_23[] = {2, 3};
static const uint8_t k_attr_6002[] = {2, 3, 6, 10};
static const uint8_t k_attr_event[] = {2, 3, 4, 6, 11, 12, 14};
static const uint8_t k_method_1[] = {1};
static const uint8_t k_attr_2_6000[] = {2};
static const uint8_t k_attr_8000[] = {2, 4, 5};
static const uint8_t k_attr_2[] = {2};
static const uint8_t k_attr_freeze[] = {3};
static const uint8_t k_method_freeze[] = {3, 4, 5, 7, 8};
static const uint8_t k_method_6000[] = {127, 128, 129, 130, 131, 132, 133, 134};
static const uint8_t k_method_6002[] = {127, 128, 129};
static const uint8_t k_method_6012[] = {1, 127, 128, 129, 130};
static const uint8_t k_method_8000[] = {127, 128, 129, 130, 131};

static const expected_object_t k_expected[] = {
    {OI_METER_ARCHIVE, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2_6000, 1, k_method_6000, 8},
    {OI_ARCHIVE_UNIT, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {OI_TASK_SCHEDULER, IC_ACQUISITION, DCU_OBJ_SHARED, k_attr_2_6000, 1, k_method_6012, 5},
    {OI_TASK_UNIT, IC_PARAM, DCU_OBJ_SHARED, k_attr_2_6000, 1, NULL, 0},
    {(uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_23, 2, NULL,
     0},
    {(uint16_t)((OAD_CURRENT >> 16) & 0xFFFF), IC_VARIABLE, DCU_OBJ_PER_METER, k_attr_23, 2, NULL,
     0},
    {(uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), IC_RECORD, DCU_OBJ_SHARED, k_attr_freeze, 1,
     k_method_freeze, 5},
    {OI_REMOTE_CONTROL, IC_CONTROL, DCU_OBJ_SINGLETON, k_attr_8000, 3, k_method_8000, 5},
    {OI_ESAM_SECURITY, IC_ESAM, DCU_OBJ_SINGLETON, NULL, 0, NULL, 0},
    {OI_CHANNEL_CONFIG, IC_PERIPHERAL, DCU_OBJ_SINGLETON, k_attr_2, 1, NULL, 0},
    {OI_METER_DISCOVERY, IC_COLLECTION, DCU_OBJ_SINGLETON, k_attr_6002, 4, k_method_6002, 3},
    {0x7000, IC_COLLECTION, DCU_OBJ_SINGLETON, NULL, 0, NULL, 0},
    {0x7010, IC_COLLECTION, DCU_OBJ_SINGLETON, NULL, 0, NULL, 0},
    {OI_VOLTAGE_DROP, IC_SUB_EVENT, DCU_OBJ_SHARED, k_attr_event, 7, k_method_1, 1},
    {OI_OVER_CURRENT, IC_SUB_EVENT, DCU_OBJ_SHARED, k_attr_event, 7, k_method_1, 1},
    {0x4000, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4001, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4002, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4003, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4004, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x401C, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x401D, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x401E, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x401F, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4020, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
    {0x4021, IC_PARAM, DCU_OBJ_SINGLETON, k_attr_2_6000, 1, NULL, 0},
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

    assert_int_equal(dcu_698_object_count(), 131);

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
    assert_int_equal(dcu_698_call(MK_OMD(0x1234, 1), NULL, NULL), EDGE_SLAVE_SRC_NOT_FOUND);

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
    dcu_discovery_t discovery;
    init_model(&inv, &poller);
    dcu_discovery_init(&discovery);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&inv, &poller, sink, NULL, &discovery, NULL, NULL, bus), 0);

    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 2), &(uint16_t){0}), -1);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 3), &(uint16_t){0}), -1);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_ARCHIVE, 18), &(uint16_t){0}), -1);

    /* 6000 全部未广告：类定义过的方法号回 DAR 5，未定义的回 DAR 4。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 1), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 2), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    /* 127 已广告，但 C API 需要解码后的参数结构；空参数如实报 DAR 7。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 200), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_ARCHIVE, 6), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);

    /* 5004：方法 3 已暴露，空 C 参数 → 7；方法 1 类定义但未暴露 → 5；方法 6 类里没有 → 4。 */
    uint16_t oi_5004 = (uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF);
    assert_int_equal(dcu_698_call(MK_OMD(oi_5004, 3), NULL, NULL), EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(oi_5004, 1), NULL, NULL), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(oi_5004, 6), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);

    assert_int_equal(dcu_698_read(MK_OAD(0x6003, 2), &(uint16_t){0}), -1);
    /* 6002 A2 是结果数组：已广告，但不是标量；A5 是 OI 定义但未广告。 */
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 2), &(uint16_t){0}), -2);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 5), &(uint16_t){0}), -1);

    /* 6002 方法 127/128/129 已广告；130 未定义 -> 4，方法 1 类定义未暴露 -> 5。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_DISCOVERY, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_DISCOVERY, 130), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_DISCOVERY, 1), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_METER_DISCOVERY, 200), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0x6003, 1), NULL, NULL), EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_discovery_destroy(&discovery);
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_discovery_oi_attributes_beyond_class_table(void **state) {
    (void)state;
    /* 7.3.11 类表到 A4；6002 的 A5..A10 在表 A.7，仍是标准定义。 */
    assert_false(dcu_698_class_defines_attr(IC_COLLECTION, 5));
    assert_true(dcu_698_oi_defines_attr(OI_METER_DISCOVERY, IC_COLLECTION, 5));
    assert_true(dcu_698_oi_defines_attr(OI_METER_DISCOVERY, IC_COLLECTION, 10));
    assert_false(dcu_698_oi_defines_attr(OI_METER_DISCOVERY, IC_COLLECTION, 11));
    assert_false(dcu_698_oi_defines_attr(OI_TASK_SCHEDULER, IC_ACQUISITION, 4));
}

static void test_dar_classification_by_class_definition(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 广告位如实回暂时失效或类型错，不伪装执行成功。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 128), NULL, NULL),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 129), NULL, NULL),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* Class 13 定义方法 1–7，产品只广告 127–131。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 1), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 7), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_REMOTE_CONTROL, 8), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);

    /* Class 21 定义方法 1–12；产品没有 ESAM 处理器，全部按类定义回 DAR 5。 */
    for (uint8_t method = 1; method <= 12; method++) {
        assert_int_equal(dcu_698_call(MK_OMD(OI_ESAM_SECURITY, method), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_698_call(MK_OMD(OI_ESAM_SECURITY, 13), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);

    /* Class 22 定义方法 1–2，F200 一个方法都不广告。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_CHANNEL_CONFIG, 1), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_CHANNEL_CONFIG, 3), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class_method_sets_exclude_standard_holes(void **state) {
    (void)state;

    assert_false(dcu_698_class_defines_method(IC_EVENT, 3));
    assert_true(dcu_698_class_defines_method(IC_EVENT, 1));
    assert_true(dcu_698_class_defines_method(IC_EVENT, 2));
    assert_true(dcu_698_class_defines_method(IC_EVENT, 4));
    assert_true(dcu_698_class_defines_method(IC_EVENT, 5));
    assert_false(dcu_698_class_defines_method(IC_EVENT, 6));

    assert_false(dcu_698_class_defines_method(IC_RECORD, 6));
    assert_true(dcu_698_class_defines_method(IC_RECORD, 1));
    assert_true(dcu_698_class_defines_method(IC_RECORD, 5));
    assert_true(dcu_698_class_defines_method(IC_RECORD, 7));
    assert_true(dcu_698_class_defines_method(IC_RECORD, 8));
    assert_false(dcu_698_class_defines_method(IC_RECORD, 9));

    assert_false(dcu_698_class_defines_method(IC_SUB_EVENT, 3));
    assert_true(dcu_698_class_defines_method(IC_SUB_EVENT, 1));
    assert_true(dcu_698_class_defines_method(IC_SUB_EVENT, 2));
    assert_true(dcu_698_class_defines_method(IC_SUB_EVENT, 4));
    assert_true(dcu_698_class_defines_method(IC_SUB_EVENT, 5));
    assert_false(dcu_698_class_defines_method(IC_SUB_EVENT, 6));

    assert_true(dcu_698_class_defines_method(IC_PARAM, 1));
    assert_true(dcu_698_class_defines_method(IC_PARAM, 2));
    assert_false(dcu_698_class_defines_method(IC_PARAM, 3));
    assert_true(dcu_698_class_defines_method(IC_ESAM, 12));
    assert_false(dcu_698_class_defines_method(IC_ESAM, 13));
    assert_false(dcu_698_class_defines_method(0, 1));
    assert_false(dcu_698_class_defines_method(IC_RECORD, 0));
}

static void test_class24_missing_method_is_undefined(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 3), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_OVER_CURRENT, 3), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 2), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_OVER_CURRENT, 4), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_OVER_CURRENT, 5), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(0x5000, 6), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0x5000, 7), NULL, NULL),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_task_scheduler_reset_and_add(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    uint32_t csd = OAD_VOLTAGE;
    assert_int_equal(dcu_poller_add_scheme(&poller, 7, 1, &csd, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 5, 60, 7), 0);
    poller.tasks[0].cursor_meter_idx = 3;
    poller.tasks[0].last_run_time = 99;
    poller.tasks[0].monitor.success_meters = 4;

    /* Class 10 方法 1 只复位运行态，不碰方案/任务配置（Clear 是 129）。 */
    int method_res = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_TASK_SCHEDULER, 1), NULL, &method_res), 0);
    assert_int_equal(method_res, 0);
    assert_non_null(dcu_poller_get_scheme(&poller, 7));
    assert_non_null(dcu_poller_task_find(&poller, 5));
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 0);
    assert_int_equal(poller.tasks[0].last_run_time, 0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);

    /* 127–130 现在是任务配置 op；没有持久化端口时如实回 DAR 2，不加方案。 */
    dcu_task_op_t op = {.method = 127, .unit_count = 0};
    dcu_task_op_t delete_op = {.method = 128, .id_count = 0};
    assert_int_equal(dcu_698_call(MK_OMD(OI_TASK_SCHEDULER, 127), &op, &method_res),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_call(MK_OMD(OI_TASK_SCHEDULER, 128), &delete_op, &method_res),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_null(dcu_poller_get_scheme(&poller, 9));

    /* 131 不在 127–130；Class 10 方法上限 3，按 DAR 4。方法 2 类定义但未广告 → DAR 5。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_TASK_SCHEDULER, 131), &op, &method_res),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_TASK_SCHEDULER, 2), NULL, &method_res),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_discovery_exposes_results_count_and_status(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_discovery_t discovery;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    dcu_discovery_init(&discovery);
    discovery.count = 2;
    discovery.status = DISCOVERY_STATUS_SCANNING;
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&inv, &poller, sink, NULL, &discovery, NULL, NULL, bus), 0);

    uint16_t count = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 3), &count), 0);
    assert_int_equal(count, 2);
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 6), &count), 0);
    assert_int_equal(count, 2);

    uint8_t status = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 10), &status), 0);
    assert_int_equal(status, DISCOVERY_STATUS_SCANNING);

    assert_int_equal(dcu_698_read(MK_OAD(OI_METER_DISCOVERY, 2), &(uint16_t){0}), -2);

    dcu_discovery_destroy(&discovery);
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_dispatch_read_write_call(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* F200 没有可落地的串口重配路径，A2/A3/A4 全部不对外。 */
    uint32_t baud = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_CHANNEL_CONFIG, 2), &baud), -1);
    uint32_t new_baud = 2400;
    assert_int_equal(dcu_698_write(MK_OAD(OI_CHANNEL_CONFIG, 2), &new_baud), -1);

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
    static const uint8_t masked[] = {5, 7, 8, 9, 10, 13};
    static const uint8_t exposed_but_not_scalar[] = {2, 3, 6, 14};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        for (size_t k = 0; k < sizeof(masked) / sizeof(masked[0]); k++) {
            uint64_t buf = 0;
            assert_int_equal(dcu_698_read(MK_OAD(ois[i], masked[k]), &buf), -1);
            assert_int_equal(dcu_698_write(MK_OAD(ois[i], masked[k]), &buf), -1);
        }
        /* 复杂属性已暴露，但对象处理器只给 A-XDR，旧标量入口按处理器返回码拒绝。 */
        for (size_t k = 0; k < sizeof(exposed_but_not_scalar) / sizeof(exposed_but_not_scalar[0]);
             k++) {
            uint64_t buf = 0;
            assert_int_equal(dcu_698_read(MK_OAD(ois[i], exposed_but_not_scalar[k]), &buf), -2);
            assert_int_equal(dcu_698_write(MK_OAD(ois[i], exposed_but_not_scalar[k]), &buf), -2);
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
                                .last_start_time = {.mono_ns = 111, .epoch_s = 1716500000},
                                .last_end_time = {.mono_ns = 222, .epoch_s = 1716500010}};
    evt_3000.records[0].is_active = true;
    init_model_with_events(&inv, &poller, &evt_3000, NULL);

    int result = -1;
    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 1), NULL, &result), 0);
    assert_int_equal(result, 0);
    assert_int_equal(evt_3000.record_count, 0);
    assert_false(evt_3000.records[0].is_active);
    assert_int_equal(evt_3000.last_start_time.mono_ns, 0);
    assert_int_equal(evt_3000.last_start_time.epoch_s, 0);
    assert_int_equal(evt_3000.last_end_time.mono_ns, 0);
    assert_int_equal(evt_3000.last_end_time.epoch_s, 0);

    /* Class 24 定义了方法 2，但 3000 只暴露复位 → 类不匹配（DAR 5）。 */
    assert_int_equal(dcu_698_call(MK_OMD(OI_VOLTAGE_DROP, 2), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

/* 类 22 的解析器用 dcu_698_oi_defines_attr 判属性是否存在，所以目录里每条
 * 实例属性上限都必须登记进 k_oi_max_attr，否则标准属性被误判 DAR 4/3
 * （D4-08a 的 F209/F20C 类缺陷）。其他接口类的属性权威在 var/param 表，
 * 不经这张表判定，不在本断言范围。 */
static void test_catalog_instance_attr_limits_are_defined(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    int missing = 0;
    for (size_t i = 0; i < sizeof(dcu698_catalog_oi_attrs) / sizeof(dcu698_catalog_oi_attrs[0]);
         i++) {
        const dcu_698_object_t *obj = dcu_698_find(dcu698_catalog_oi_attrs[i].oi);
        if (!obj || obj->class_id != IC_PERIPHERAL) continue;
        if (!dcu_698_oi_defines_attr(obj->oi, obj->class_id,
                                     dcu698_catalog_oi_attrs[i].max_attr)) {
            print_message("OI 0x%04X class %u: catalog max_attr %u not registered\n", obj->oi,
                          obj->class_id, dcu698_catalog_oi_attrs[i].max_attr);
            missing++;
        }
    }
    assert_int_equal(missing, 0);
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

static void test_discovery_snapshot_and_clear(void **state) {
    (void)state;
    dcu_discovery_t disc;
    dcu_discovery_init(&disc);

    uint8_t status = 99;
    uint16_t count = 99;
    dcu_discovery_snapshot(&disc, &status, &count);
    assert_int_equal(status, DISCOVERY_STATUS_IDLE);
    assert_int_equal(count, 0);

    disc.count = 5;
    dcu_discovery_clear(&disc);
    dcu_discovery_snapshot(&disc, &status, &count);
    assert_int_equal(count, 0);
    assert_int_equal(status, DISCOVERY_STATUS_IDLE);

    dcu_discovery_destroy(&disc);
}

static void test_class14_15_16_stats_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    static const struct {
        uint16_t oi;
        uint8_t class_id;
    } rows[] = {
        {0x2100, 14}, {0x2101, 14}, {0x2102, 14}, {0x2103, 14}, {0x2104, 14},
        {0x2110, 15}, {0x2111, 15}, {0x2112, 15}, {0x2113, 15}, {0x2114, 15},
        {0x2120, 16}, {0x2121, 16}, {0x2122, 16}, {0x2123, 16}, {0x2124, 16},
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(rows[i].oi);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, rows[i].class_id);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class17_display_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    static const uint16_t ois[] = {0xF300, 0xF301};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(ois[i]);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 17);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class11_collection_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    static const uint16_t ois[] = {0x7000, 0x7010};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(ois[i]);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, IC_COLLECTION);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
    }

    for (uint8_t method = 127; method <= 129; method++) {
        assert_int_equal(dcu_698_call(MK_OMD(0x7000, method), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_698_call(MK_OMD(0x7000, 130), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    for (uint8_t method = 127; method <= 130; method++) {
        assert_int_equal(dcu_698_call(MK_OMD(0x7010, method), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_698_call(MK_OMD(0x7010, 131), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class12_pulse_row_is_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    const dcu_698_object_t *obj = dcu_698_find(0x2401);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, 12);
    assert_mask_matches(obj->attr_mask, NULL, 0);
    assert_mask_matches(obj->method_mask, NULL, 0);
    for (uint8_t method = 1; method <= 4; method++) {
        assert_int_equal(dcu_698_call(MK_OMD(0x2401, method), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_698_call(MK_OMD(0x2401, 5), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class25_26_comm_rows(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 表196 定义 Class 25 的 A1..A11、方法 1（复位）；产品没有任何无线模块来源。 */
    static const uint16_t wireless[] = {0x4500, 0x4501};
    for (size_t i = 0; i < sizeof(wireless) / sizeof(wireless[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(wireless[i]);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 25);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
        assert_int_equal(dcu_698_call(MK_OMD(wireless[i], 1), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
        assert_int_equal(dcu_698_call(MK_OMD(wireless[i], 2), NULL, NULL),
                         EDGE_SLAVE_SRC_UNDEFINED);
    }

    /* 表199 定义 Class 26 的 A1..A5、方法 1；平台只给 4510 的 MAC（A5）。 */
    static const uint8_t k_attr_mac[] = {5};
    const dcu_698_object_t *first = dcu_698_find(0x4510);
    assert_non_null(first);
    assert_int_equal(first->class_id, 26);
    assert_mask_matches(first->attr_mask, k_attr_mac, 1);
    for (uint16_t oi = 0x4511; oi <= 0x4517; oi++) {
        const dcu_698_object_t *obj = dcu_698_find(oi);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 26);
        assert_mask_matches(obj->attr_mask, NULL, 0);
    }
    for (uint16_t oi = 0x4510; oi <= 0x4517; oi++) {
        assert_int_equal(dcu_698_call(MK_OMD(oi, 1), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
        assert_int_equal(dcu_698_call(MK_OMD(oi, 2), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class19_devmgr_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 表178：Class 19 定义 A1..A10、方法 1..6；产品没有设备管理/复位/初始化端口，
     * 行只广告存在性。4307..4309 的 OI 专属方法 127..129 由标准定义、无处理器。 */
    static const uint16_t ois[] = {0x4300, 0x4307, 0x4308, 0x4309};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(ois[i]);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 19);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
        for (uint8_t method = 1; method <= 6; method++) {
            assert_int_equal(dcu_698_call(MK_OMD(ois[i], method), NULL, NULL),
                             EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
        assert_int_equal(dcu_698_call(MK_OMD(ois[i], 7), NULL, NULL),
                         EDGE_SLAVE_SRC_UNDEFINED);
    }
    for (uint16_t oi = 0x4307; oi <= 0x4309; oi++) {
        for (uint8_t method = 127; method <= 129; method++) {
            assert_int_equal(dcu_698_call(MK_OMD(oi, method), NULL, NULL),
                             EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
        assert_int_equal(dcu_698_call(MK_OMD(oi, 130), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
    assert_int_equal(dcu_698_call(MK_OMD(0x4300, 127), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class23_group_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 表190：Class 23 定义 A1..A18、方法 1..5；产品没有总加组配置/总加值/滑差来源，
     * 8 个总加组 OI 只广告对象存在性。 */
    for (uint16_t oi = 0x2301; oi <= 0x2308; oi++) {
        const dcu_698_object_t *obj = dcu_698_find(oi);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 23);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
        for (uint8_t method = 1; method <= 5; method++) {
            assert_int_equal(dcu_698_call(MK_OMD(oi, method), NULL, NULL),
                             EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
        assert_int_equal(dcu_698_call(MK_OMD(oi, 6), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class18_file_rows_are_empty(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    /* 表175/表A.11：Class 18 定义 A1..A3、方法 1..8；F000 另有 A4/A5，
     * F001/F002 另有 A4，F001 另有方法 9/10。产品没有文件传输/DFU 来源。 */
    static const uint16_t ois[] = {0xF000, 0xF001, 0xF002};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        const dcu_698_object_t *obj = dcu_698_find(ois[i]);
        assert_non_null(obj);
        assert_int_equal(obj->class_id, 18);
        assert_mask_matches(obj->attr_mask, NULL, 0);
        assert_mask_matches(obj->method_mask, NULL, 0);
        for (uint8_t method = 1; method <= 8; method++) {
            assert_int_equal(dcu_698_call(MK_OMD(ois[i], method), NULL, NULL),
                             EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
    }
    for (uint8_t method = 9; method <= 10; method++) {
        assert_int_equal(dcu_698_call(MK_OMD(0xF001, method), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_698_call(MK_OMD(0xF001, 11), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0xF000, 9), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0xF002, 9), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);

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
        cmocka_unit_test_setup_teardown(test_catalog_instance_attr_limits_are_defined, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_mask_bitmap_supports_wide_ids, setup, teardown),
        cmocka_unit_test_setup_teardown(test_mask_rejects_missing_attr_and_method, setup, teardown),
        cmocka_unit_test_setup_teardown(test_discovery_oi_attributes_beyond_class_table, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_task_scheduler_reset_and_add, setup, teardown),
        cmocka_unit_test_setup_teardown(test_discovery_exposes_results_count_and_status, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_dar_classification_by_class_definition, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_class24_missing_method_is_undefined, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_class_method_sets_exclude_standard_holes, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_dispatch_read_write_call, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class24_attr_reads, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class24_attr_writes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_event_class7_attr_numbers_are_not_served, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_event_method1_resets_records, setup, teardown),
        cmocka_unit_test_setup_teardown(test_facade_and_table_api_agree, setup, teardown),
        cmocka_unit_test_setup_teardown(test_discovery_snapshot_and_clear, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class14_15_16_stats_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class17_display_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class11_collection_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class12_pulse_row_is_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class25_26_comm_rows, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class19_devmgr_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class23_group_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class18_file_rows_are_empty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_reset_returns_to_uninitialized, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
