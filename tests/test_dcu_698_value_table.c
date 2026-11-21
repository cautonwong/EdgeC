#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "datahub.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dcu_router.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define TS_NS (1234567890ULL * 1000000ULL)

static int mock_shadow(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
    (void)ctx;
    (void)device_id;
    pipe_msg_t *msg = (pipe_msg_t *)out_msg;
    memset(msg, 0, sizeof(*msg));

    double value;
    switch (tag_id) {
        case OAD_ACTIVE_ENERGY:
            value = 12345.0;
            break;
        case OAD_REACTIVE_ENERGY:
            value = 6789.0;
            break;
        case OAD_VOLTAGE:
            value = 220.5;
            break;
        case OAD_CURRENT:
            value = 4.95;
            break;
        case OAD_ACTIVE_POWER_TOTAL:
            value = 1234.5;
            break;
        case OAD_POWER_FACTOR_TOTAL:
            value = 0.95;
            break;
        case OAD_GRID_FREQUENCY:
            value = 50.01;
            break;
        default:
            return -1;
    }
    msg->tag_id = tag_id;
    msg->ts = TS_NS;
    msg->quality = 0;
    msg->payload.val.f = value;
    return 0;
}

static int mock_count_freezes(void *ctx, uint16_t *out) {
    (void)ctx;
    *out = 7;
    return 0;
}

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;
static dcu_event_obj_t g_evt_3000;

static void init_model(void) {
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    assert_int_equal(dcu_inventory_add(&g_inv, "000000000001", "DLT645", 2400, 22), 0);

    memset(&g_evt_3000, 0, sizeof(g_evt_3000));
    g_evt_3000.oi = OI_VOLTAGE_DROP;
    g_evt_3000.record_count = 3;
    g_evt_3000.threshold = 5.0f;
    g_evt_3000.report_flag = 1;
    g_evt_3000.is_enabled = true;

    router_sink_ops_t sink = {.ctx = NULL, .count_freezes = mock_count_freezes};
    router_bus_ops_t bus = {.msg_size = sizeof(pipe_msg_t), .read_shadow = mock_shadow};
    assert_int_equal(
        dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, &g_evt_3000, NULL, bus), 0);
}

static void teardown_model(void) {
    dcu_698_object_model_reset();
    dcu_inventory_destroy(&g_inv);
    dcu_poller_destroy(&g_poller);
}

static void assert_value(uint32_t oad, uint8_t want_tag, double want_value) {
    dcu_value_t v;
    memset(&v, 0, sizeof(v));
    assert_int_equal(dcu_698_value_for_oad(oad, &v), 0);
    assert_int_equal(v.type_tag, want_tag);
    assert_int_equal(v.width, dcu_698_tag_width(want_tag));
    assert_float_equal((float)v.value, (float)want_value, 0.001f);
}

static void test_variable_ois_type_tags(void **state) {
    (void)state;
    init_model();

    assert_value(OAD_ACTIVE_ENERGY, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 12345.0);
    assert_value(OAD_REACTIVE_ENERGY, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 6789.0);
    assert_value(OAD_VOLTAGE, DCU_698_TAG_FLOAT32, 220.5);
    assert_value(OAD_CURRENT, DCU_698_TAG_FLOAT32, 4.95);
    assert_value(OAD_ACTIVE_POWER_TOTAL, DCU_698_TAG_FLOAT32, 1234.5);
    assert_value(OAD_POWER_FACTOR_TOTAL, DCU_698_TAG_LONG, 0.95);
    assert_value(OAD_GRID_FREQUENCY, DCU_698_TAG_LONG_UNSIGNED, 50.01);
    assert_value(MK_OAD(0x0010, 4), DCU_698_TAG_LONG64_UNSIGNED, 12345.0);

    teardown_model();
}

static void test_object_model_scalar_type_tags(void **state) {
    (void)state;
    init_model();

    assert_value(MK_OAD(OI_METER_ARCHIVE, 2), DCU_698_TAG_LONG_UNSIGNED, 1.0);
    assert_value(MK_OAD(OI_TASK_SCHEDULER, 2), DCU_698_TAG_LONG_UNSIGNED, 0.0);
    assert_value(MK_OAD((uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), 2),
                 DCU_698_TAG_LONG_UNSIGNED, 7.0);
    assert_value(MK_OAD(OI_CHANNEL_CONFIG, 2), DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 9600.0);
    assert_value(MK_OAD(OI_CHANNEL_CONFIG, 3), DCU_698_TAG_UNSIGNED, 0.0);
    assert_value(MK_OAD(OI_VOLTAGE_DROP, 4), DCU_698_TAG_LONG_UNSIGNED, 3.0);
    assert_value(MK_OAD(OI_VOLTAGE_DROP, 5), DCU_698_TAG_LONG_UNSIGNED, MAX_EVENT_RECORDS);
    assert_value(MK_OAD(OI_VOLTAGE_DROP, 6), DCU_698_TAG_FLOAT32, 5.0);
    assert_value(MK_OAD(OI_VOLTAGE_DROP, 8), DCU_698_TAG_UNSIGNED, 1.0);
    assert_value(MK_OAD(OI_VOLTAGE_DROP, 9), DCU_698_TAG_BOOLEAN, 1.0);

    teardown_model();
}

static void test_scalar_callback_type_and_width(void **state) {
    (void)state;
    init_model();

    edge_point_value_t p;
    memset(&p, 0, sizeof(p));
    assert_int_equal(dcu_north_resolve_scalar(NULL, MK_OAD(OI_METER_ARCHIVE, 2), NULL, 0, &p), 0);
    assert_int_equal(p.type, DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal(p.width, 2);
    assert_float_equal((float)p.value, 1.0f, 0.001f);

    memset(&p, 0, sizeof(p));
    assert_int_equal(dcu_north_resolve_scalar(NULL, OAD_VOLTAGE, NULL, 0, &p), 0);
    assert_int_equal(p.type, DCU_698_TAG_FLOAT32);
    assert_int_equal(p.width, 4);

    teardown_model();
}

static void test_structure_is_not_scalar(void **state) {
    (void)state;
    init_model();

    dcu_value_t v;
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_VOLTAGE_DROP, 2), &v),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    edge_point_value_t p;
    assert_int_equal(dcu_north_resolve_scalar(NULL, MK_OAD(OI_VOLTAGE_DROP, 2), NULL, 0, &p),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    teardown_model();
}

static void test_scalar_and_axdr_agree_on_width_one_types(void **state) {
    (void)state;
    init_model();

    const uint32_t oads[] = {MK_OAD(OI_VOLTAGE_DROP, 8), MK_OAD(OI_VOLTAGE_DROP, 9)};
    const uint8_t tags[] = {DCU_698_TAG_UNSIGNED, DCU_698_TAG_BOOLEAN};
    for (size_t i = 0; i < sizeof(oads) / sizeof(oads[0]); i++) {
        edge_point_value_t p;
        memset(&p, 0, sizeof(p));
        assert_int_equal(dcu_north_resolve_scalar(NULL, oads[i], NULL, 0, &p), 0);
        assert_int_equal(p.type, tags[i]);
        assert_int_equal(p.width, 1);

        uint8_t buf[EDGE_SLAVE_AXDR_MAX];
        size_t len = 0;
        assert_int_equal(dcu_north_resolve_axdr(NULL, oads[i], buf, sizeof(buf), &len), 0);
        assert_int_equal(len, 2);
        assert_int_equal(buf[0], tags[i]);
        assert_int_equal(buf[1], 1);
    }

    teardown_model();
}

static void test_scalar_rejects_64bit_raw(void **state) {
    (void)state;
    init_model();

    /* 端口 raw 是 int32；LONG64 只能走 A-XDR，不能在标量回调里截断冒充。 */
    edge_point_value_t p;
    assert_int_equal(dcu_north_resolve_scalar(NULL, MK_OAD(0x0010, 4), NULL, 0, &p),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x0010, 4), buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DCU_698_TAG_LONG64_UNSIGNED);

    teardown_model();
}

static void test_resolve_axdr_for_north_list_oads(void **state) {
    (void)state;
    init_model();

    const uint32_t oads[] = {OAD_VOLTAGE, OAD_CURRENT, OAD_ACTIVE_POWER_TOTAL,
                             MK_OAD(OI_METER_ARCHIVE, 2), MK_OAD(OI_TASK_SCHEDULER, 2),
                             OAD_DAILY_FREEZE};
    for (size_t i = 0; i < sizeof(oads) / sizeof(oads[0]); i++) {
        uint8_t buf[EDGE_SLAVE_AXDR_MAX];
        size_t len = 0;
        assert_int_equal(dcu_north_resolve_axdr(NULL, oads[i], buf, sizeof(buf), &len), 0);
        assert_true(len > 0);
    }

    teardown_model();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_variable_ois_type_tags),
        cmocka_unit_test(test_object_model_scalar_type_tags),
        cmocka_unit_test(test_scalar_callback_type_and_width),
        cmocka_unit_test(test_scalar_and_axdr_agree_on_width_one_types),
        cmocka_unit_test(test_scalar_rejects_64bit_raw),
        cmocka_unit_test(test_structure_is_not_scalar),
        cmocka_unit_test(test_resolve_axdr_for_north_list_oads),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
