/**
 * @file test_dcu_698_var.c
 * @brief 变量类（Class 1/3/4/6）的标准形状与寻址（ADR-0016）。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "datahub.h"
#include "dcu_comm_info.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define TS_NS (1234567890ULL * 1000000ULL)

static int mock_read_shadow(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
    (void)ctx;
    if (device_id > 1) return -1;
    pipe_msg_t *msg = (pipe_msg_t *)out_msg;
    memset(msg, 0, sizeof(*msg));

    double value;
    switch (tag_id) {
        case OAD_VOLTAGE:
            value = device_id == 0 ? 231.7 : 240.0;
            break;
        case TAG_VOLTAGE_B:
            value = 230.1;
            break;
        case TAG_VOLTAGE_C:
            value = 229.5;
            break;
        case OAD_CURRENT:
            value = 4.951;
            break;
        case TAG_CURRENT_B:
            value = 5.0;
            break;
        case TAG_CURRENT_C:
            value = 5.05;
            break;
        case OAD_ACTIVE_POWER_TOTAL:
            value = 1234.5;
            break;
        case TAG_ACTIVE_POWER_A:
            value = 100.0;
            break;
        case TAG_ACTIVE_POWER_B:
            value = 200.0;
            break;
        case TAG_ACTIVE_POWER_C:
            value = 300.0;
            break;
        case OAD_ACTIVE_ENERGY:
            value = 12345.0;
            break;
        case OAD_REACTIVE_ENERGY:
            value = 6789.0;
            break;
        case OAD_POWER_FACTOR_TOTAL:
            value = 0.95;
            break;
        case OAD_GRID_FREQUENCY:
            value = device_id == 0 ? 50.01 : 49.98;
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
                            .read_shadow = read_shadow};
    assert_int_equal(dcu_698_object_model_init(inv, poller, sink, NULL, NULL, NULL, NULL, bus), 0);
}

static size_t encode(uint32_t oad, uint8_t *buf, size_t cap) {
    size_t len = 0;
    assert_int_equal(dcu_698_var_encode_axdr(oad, buf, cap, &len), 0);
    return len;
}

static void expect_axdr(uint32_t oad, const uint8_t *want, size_t want_len) {
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = encode(oad, buf, sizeof(buf));
    assert_int_equal(len, want_len);
    assert_memory_equal(buf, want, want_len);
}

static void test_class3_voltage_array_and_scaler(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const uint8_t voltage[] = {0x01, 0x03, 0x12, 0x09, 0x0D, 0x12, 0x08, 0xFD, 0x12, 0x08, 0xF7};
    expect_axdr(OAD_VOLTAGE, voltage, sizeof(voltage));

    const uint8_t scaler[] = {0x02, 0x02, 0x0F, 0xFF, 0x16, 0x26};
    expect_axdr(MK_OAD(0x2000, 3), scaler, sizeof(scaler));

    /* 元素索引选表：slot 1 的 A 相是 240.0 V。 */
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = encode(0x20000201U, buf, sizeof(buf));
    assert_int_equal(len, sizeof(voltage));
    assert_int_equal(buf[2], 0x12);
    assert_int_equal(buf[3], 0x09);
    assert_int_equal(buf[4], 0x60);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class3_current_array(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const uint8_t current[] = {0x01, 0x03, 0x05, 0x00, 0x00, 0x13, 0x57, 0x05, 0x00, 0x00,
                               0x13, 0x88, 0x05, 0x00, 0x00, 0x13, 0xBA};
    expect_axdr(OAD_CURRENT, current, sizeof(current));

    const uint8_t scaler[] = {0x02, 0x02, 0x0F, 0xFD, 0x16, 0x24};
    expect_axdr(MK_OAD(0x2001, 3), scaler, sizeof(scaler));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class4_power_array_and_power_factor(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const uint8_t power[] = {0x01, 0x04, 0x05, 0x00, 0x00, 0x30, 0x39, 0x05, 0x00, 0x00, 0x03,
                             0xE8, 0x05, 0x00, 0x00, 0x07, 0xD0, 0x05, 0x00, 0x00, 0x0B, 0xB8};
    expect_axdr(OAD_ACTIVE_POWER_TOTAL, power, sizeof(power));

    const uint8_t scaler[] = {0x02, 0x02, 0x0F, 0xFF, 0x16, 0x1B};
    expect_axdr(MK_OAD(0x2004, 3), scaler, sizeof(scaler));

    const uint8_t pf[] = {0x01, 0x01, 0x10, 0x03, 0xB6};
    expect_axdr(OAD_POWER_FACTOR_TOTAL, pf, sizeof(pf));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class6_frequency_is_scalar_with_scale(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    dcu_value_t v;
    memset(&v, 0, sizeof(v));
    assert_int_equal(dcu_698_var_read_value(OAD_GRID_FREQUENCY, &v), 0);
    assert_int_equal(v.type_tag, DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal(v.raw, 5001);
    assert_float_equal((float)v.value, 50.01f, 0.001f);

    const uint8_t scaler[] = {0x02, 0x02, 0x0F, 0xFE, 0x16, 0x2F};
    expect_axdr(MK_OAD(0x200F, 3), scaler, sizeof(scaler));

    /* 元素索引选表：slot 1 的频率是 49.98 → 4998。 */
    uint16_t slot1 = 0;
    const dcu_698_object_t *obj = dcu_698_find(0x200F);
    assert_non_null(obj);
    assert_int_equal(dcu_698_var_read(obj, 2, 1, &slot1), 0);
    assert_int_equal(slot1, 4998);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class1_energy_arrays(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const uint8_t energy[] = {0x01, 0x01, 0x06, 0x00, 0x12, 0xD6, 0x44};
    expect_axdr(OAD_ACTIVE_ENERGY, energy, sizeof(energy));

    const uint8_t high[] = {0x01, 0x01, 0x15, 0x00, 0x00, 0x00,
                            0x00, 0x07, 0x5B, 0xB2, 0x90};
    expect_axdr(MK_OAD(0x0010, 4), high, sizeof(high));

    const uint8_t scaler2[] = {0x02, 0x02, 0x0F, 0xFE, 0x16, 0x21};
    expect_axdr(MK_OAD(0x0010, 3), scaler2, sizeof(scaler2));
    const uint8_t scaler4[] = {0x02, 0x02, 0x0F, 0xFC, 0x16, 0x21};
    expect_axdr(MK_OAD(0x0010, 5), scaler4, sizeof(scaler4));

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_wrong_slot_and_attr(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_698_var_encode_axdr(0x200002FAU, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    /* 变量类按类定义分类：A1 类定义但未实现 -> DAR 5，A4 类表外 -> DAR 4。 */
    assert_int_equal(dcu_698_var_encode_axdr(MK_OAD(0x2000, 1), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_var_encode_axdr(MK_OAD(0x2000, 4), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);

    dcu_value_t v;
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(0x2000, 3), &v),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_value_for_oad(OAD_VOLTAGE, &v), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_variable_attr_dar_classification(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    dcu_value_t v;
    /* 0010 A1：Class 1 定义 A1..A5，产品未实现 A1 -> 已定义未实现 DAR 5。 */
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(0x0010, 1), &v),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    /* 0010 A6：Class 1 类表只到 A5 -> 未定义 DAR 4。 */
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(0x0010, 6), &v), EDGE_SLAVE_SRC_UNDEFINED);
    /* 未知 OI 不在对象表 -> DAR 6，不当成类外属性。 */
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(0x9999, 2), &v), EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_shadow_missing_is_not_found(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow_fail);

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_698_var_encode_axdr(OAD_VOLTAGE, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_value_t v;
    assert_int_equal(dcu_698_var_read_value(OAD_VOLTAGE, &v), -1);
    assert_int_equal(dcu_698_var_read_value(OAD_GRID_FREQUENCY, &v), -1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_class_ids_and_masks(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    const dcu_698_object_t *obj = dcu_698_find(0x0010);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_ENERGY);
    assert_true(dcu_mask_test(obj->attr_mask, 2));
    assert_true(dcu_mask_test(obj->attr_mask, 4));

    obj = dcu_698_find(0x2004);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_POWER);

    obj = dcu_698_find(0x200F);
    assert_non_null(obj);
    assert_int_equal(obj->class_id, IC_DATA_VARIABLE);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

static void test_4510_a5_scalar_path_refuses_octet_string(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller, mock_read_shadow);

    dcu_comm_info_t info = {0};
    info.mac_present = true;
    info.mac[0] = 0x02;
    info.mac[1] = 0x11;
    info.mac[2] = 0x22;
    info.mac[3] = 0x33;
    info.mac[4] = 0x44;
    info.mac[5] = 0x55;
    dcu_comm_info_install(&info);

    dcu_value_t v;
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_ETHERNET_COMM_FIRST, 5), buf, sizeof(buf), &len),
        0);
    static const uint8_t want[] = {DLT698_TAG_OCTET_STRING, 6, 0x02, 0x11,
                                   0x22,                   0x33, 0x44, 0x55};
    assert_int_equal(len, sizeof(want));
    assert_memory_equal(buf, want, sizeof(want));

    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_ETHERNET_COMM_FIRST, 5), &v),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_comm_info_reset();
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_ETHERNET_COMM_FIRST, 5), &v),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_ETHERNET_COMM_FIRST, 5), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);

    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_ETHERNET_COMM_FIRST, 6), &v),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_WIRELESS_COMM_FIRST, 1), &v),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_value_for_oad(MK_OAD(OI_ETHERNET_COMM_FIRST + 1, 5), &v),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_class3_voltage_array_and_scaler, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class3_current_array, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class4_power_array_and_power_factor, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class6_frequency_is_scalar_with_scale, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_class1_energy_arrays, setup, teardown),
        cmocka_unit_test_setup_teardown(test_wrong_slot_and_attr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_variable_attr_dar_classification, setup, teardown),
        cmocka_unit_test_setup_teardown(test_shadow_missing_is_not_found, setup, teardown),
        cmocka_unit_test_setup_teardown(test_class_ids_and_masks, setup, teardown),
        cmocka_unit_test_setup_teardown(test_4510_a5_scalar_path_refuses_octet_string, setup,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
