/**
 * @file test_dcu_698_var.c
 * @brief D1-04：变量类（Class 1/3/4/6）读取与类型标签。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "datahub.h"
#include "dcu_698_object_model.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define OI_GRID_FREQUENCY_200F 0x200F
#define TS_NS (1234567890ULL * 1000000ULL)

static int mock_read_shadow(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
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

static int mock_read_shadow_fail(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
    (void)ctx;
    (void)tag_id;
    (void)device_id;
    (void)out_msg;
    return -1;
}

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

static void init_model(dcu_inventory_t *inv, dcu_poller_t *poller,
                       int (*read_shadow)(void *, uint32_t, uint32_t, void *)) {
    assert_int_equal(dcu_inventory_init(inv), 0);
    assert_int_equal(dcu_poller_init(poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {.ctx = NULL,
                            .msg_size = sizeof(pipe_msg_t),
                            .publish_realtime = NULL,
                            .read_shadow = read_shadow};
    assert_int_equal(dcu_698_object_model_init(inv, poller, sink, NULL, NULL, NULL, NULL, bus), 0);
}

static void assert_value(uint32_t oad, double want_value, uint8_t want_tag) {
    dcu_value_t v;
    memset(&v, 0, sizeof(v));
    assert_int_equal(dcu_698_var_read_value(oad, &v), 0);
    assert_float_equal((float)v.value, (float)want_value, 0.001f);
    assert_int_equal(v.type_tag, want_tag);
    assert_int_equal(v.ts_ms, TS_NS / 1000000ULL);
    assert_int_equal(v.quality, 0);
    assert_int_equal(v.raw, (int64_t)want_value);
}

static void test_six_ois_values_and_type_tags(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    assert_value(OAD_ACTIVE_ENERGY, 12345.0, DCU_698_TAG_DOUBLE_LONG_UNSIGNED);
    assert_value(OAD_REACTIVE_ENERGY, 6789.0, DCU_698_TAG_DOUBLE_LONG_UNSIGNED);
    assert_value(OAD_VOLTAGE, 220.5, DCU_698_TAG_FLOAT32);
    assert_value(OAD_CURRENT, 4.95, DCU_698_TAG_FLOAT32);
    assert_value(OAD_ACTIVE_POWER_TOTAL, 1234.5, DCU_698_TAG_FLOAT32);
    assert_value(MK_OAD(OI_GRID_FREQUENCY_200F, 2), 50.01, DCU_698_TAG_LONG_UNSIGNED);

    /* 对象表行必须带上正确的 class 与属性 mask。 */
    const dcu_698_object_t *obj = dcu_698_find(0x0010);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_ENERGY);
    assert_true(dcu_mask_test(obj->attr_mask, 2));
    assert_true(dcu_mask_test(obj->attr_mask, 4));

    obj = dcu_698_find(0x0020);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_ENERGY);

    obj = dcu_698_find(0x2004);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_POWER);
    assert_true(dcu_mask_test(obj->attr_mask, 2));

    obj = dcu_698_find(0x200A);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_POWER);

    obj = dcu_698_find(OI_GRID_FREQUENCY_200F);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_DATA_VARIABLE);
    assert_true(dcu_mask_test(obj->attr_mask, 2));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_shadow_missing_returns_minus_one(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow_fail);

    dcu_value_t v;
    assert_int_equal(dcu_698_var_read_value(OAD_VOLTAGE, &v), -1);
    float f = 0.0f;
    assert_int_equal(dcu_698_read(OAD_VOLTAGE, &f), -1);

    /* 未登记的 OI 也必须拒绝。 */
    assert_int_equal(dcu_698_var_read_value(MK_OAD(0x9999, 2), &v), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_unsupported_attr_rejected_by_mask(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    float f = 0.0f;
    assert_int_equal(dcu_698_read(MK_OAD(0x2000, 3), &f), -1);
    dcu_value_t v;
    assert_int_equal(dcu_698_var_read_value(MK_OAD(0x2000, 3), &v), -1);

    /* Class 1 的属性 4 已登记，应可读；属性 3 未登记。 */
    assert_int_equal(dcu_698_var_read_value(MK_OAD(0x0010, 4), &v), 0);
    assert_int_equal(dcu_698_var_read_value(MK_OAD(0x0010, 3), &v), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_legacy_scalar_output_types(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    float voltage = 0.0f;
    assert_int_equal(dcu_698_read(OAD_VOLTAGE, &voltage), 0);
    assert_float_equal(voltage, 220.5f, 0.001f);

    uint32_t energy = 0;
    assert_int_equal(dcu_698_read(OAD_ACTIVE_ENERGY, &energy), 0);
    assert_int_equal(energy, 12345U);

    uint64_t energy_hp = 0;
    assert_int_equal(dcu_698_read(MK_OAD(0x0010, 4), &energy_hp), 0);
    assert_int_equal(energy_hp, 12345U);

    uint16_t freq = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_GRID_FREQUENCY_200F, 2), &freq), 0);
    assert_int_equal(freq, 50);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_energy_attr2_and_attr4_type_tags(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const uint32_t oads[] = {OAD_ACTIVE_ENERGY, OAD_REACTIVE_ENERGY};
    for (size_t i = 0; i < sizeof(oads) / sizeof(oads[0]); i++) {
        uint16_t oi = (uint16_t)(oads[i] >> 16);
        dcu_value_t attr2;
        dcu_value_t attr4;
        assert_int_equal(dcu_698_var_read_value(MK_OAD(oi, 2), &attr2), 0);
        assert_int_equal(dcu_698_var_read_value(MK_OAD(oi, 4), &attr4), 0);
        assert_int_equal(attr2.type_tag, DCU_698_TAG_DOUBLE_LONG_UNSIGNED);
        assert_int_equal(attr4.type_tag, DCU_698_TAG_LONG64_UNSIGNED);
        assert_float_equal((float)attr2.value, (float)attr4.value, 0.001f);
    }

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_power_factor_and_frequency_are_separated(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    /* 宏必须落在标准 OI：200A 功率因数、200F 频率。 */
    assert_int_equal(OAD_POWER_FACTOR_TOTAL, 0x200A0200U);
    assert_int_equal(OAD_GRID_FREQUENCY, 0x200F0200U);

    dcu_value_t pf;
    dcu_value_t freq;
    assert_int_equal(dcu_698_var_read_value(OAD_POWER_FACTOR_TOTAL, &pf), 0);
    assert_int_equal(dcu_698_var_read_value(OAD_GRID_FREQUENCY, &freq), 0);
    assert_float_equal((float)pf.value, 0.95f, 0.001f);
    assert_float_equal((float)freq.value, 50.01f, 0.001f);
    assert_int_equal(pf.type_tag, DCU_698_TAG_LONG);
    assert_int_equal(freq.type_tag, DCU_698_TAG_LONG_UNSIGNED);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_repeat_reads_are_consistent(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    dcu_value_t a;
    dcu_value_t b;
    assert_int_equal(dcu_698_var_read_value(OAD_CURRENT, &a), 0);
    assert_int_equal(dcu_698_var_read_value(OAD_CURRENT, &b), 0);
    assert_memory_equal(&a, &b, sizeof(a));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_six_ois_values_and_type_tags, setup, teardown),
        cmocka_unit_test_setup_teardown(test_shadow_missing_returns_minus_one, setup, teardown),
        cmocka_unit_test_setup_teardown(test_unsupported_attr_rejected_by_mask, setup, teardown),
        cmocka_unit_test_setup_teardown(test_legacy_scalar_output_types, setup, teardown),
        cmocka_unit_test_setup_teardown(test_energy_attr2_and_attr4_type_tags, setup, teardown),
        cmocka_unit_test_setup_teardown(test_power_factor_and_frequency_are_separated, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_repeat_reads_are_consistent, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
