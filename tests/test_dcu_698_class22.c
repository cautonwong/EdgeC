#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

/* 附录 A.12：F200..F20C 全部属于接口类 22（F210 是类 8）。 */
typedef struct {
    uint16_t oi;
    uint8_t attrs[3];
    size_t count;
} class22_row_t;

static const class22_row_t k_class22[] = {
    {OI_CHANNEL_CONFIG, {2}, 1},         {OI_RS485_PORT, {2}, 1},     {OI_INFRARED_PORT, {2}, 1},
    {OI_DIGITAL_INPUT, {2, 4}, 2},       {OI_DC_ANALOG, {2, 4}, 2},   {OI_RELAY_OUTPUT, {2}, 1},
    {OI_ALARM_OUTPUT, {2, 4}, 2},        {OI_MULTI_TERMINAL, {2}, 1}, {OI_AC_SAMPLING, {2}, 1},
    {OI_LOCAL_MODULE, {2, 5, 6}, 3},     {OI_PULSE_INPUT, {2}, 1},    {OI_BLUETOOTH, {2}, 1},
    {OI_WIRELESS_PRIVATE, {2, 4, 5}, 3},
};

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;

static int setup_model(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, NULL, NULL, bus), 0);
    return 0;
}

static int teardown_model(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    return 0;
}

static bool _advertised(const class22_row_t *row, uint8_t attr_id) {
    for (size_t i = 0; i < row->count; i++) {
        if (row->attrs[i] == attr_id) return true;
    }
    return false;
}

static void test_advertised_attrs_get_returns_dar2(void **state) {
    (void)state;
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    for (size_t r = 0; r < sizeof(k_class22) / sizeof(k_class22[0]); r++) {
        const class22_row_t *row = &k_class22[r];
        for (size_t a = 0; a < row->count; a++) {
            size_t len = 0;
            assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(row->oi, row->attrs[a]), buf,
                                                    sizeof(buf), &len),
                             EDGE_SLAVE_SRC_TEMPORARY);
        }
    }
}

static void test_unadvertised_attrs_dar_by_class_definition(void **state) {
    (void)state;
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    for (size_t r = 0; r < sizeof(k_class22) / sizeof(k_class22[0]); r++) {
        const class22_row_t *row = &k_class22[r];
        for (uint8_t attr = 1; attr <= 10; attr++) {
            if (_advertised(row, attr)) continue;
            size_t len = 0;
            int want = attr <= 4 ? EDGE_SLAVE_SRC_CLASS_MISMATCH : EDGE_SLAVE_SRC_UNDEFINED;
            assert_int_equal(
                dcu_north_resolve_axdr(NULL, MK_OAD(row->oi, attr), buf, sizeof(buf), &len), want);
        }
    }
}

static void test_scalar_port_refuses_every_advertised_attr(void **state) {
    (void)state;
    dcu_value_t v;
    for (size_t r = 0; r < sizeof(k_class22) / sizeof(k_class22[0]); r++) {
        const class22_row_t *row = &k_class22[r];
        for (size_t a = 0; a < row->count; a++) {
            assert_int_equal(dcu_698_value_for_oad(MK_OAD(row->oi, row->attrs[a]), &v),
                             EDGE_SLAVE_SRC_TYPE_MISMATCH);
        }
    }
}

static void test_advertised_attrs_are_read_only(void **state) {
    (void)state;
    const uint8_t baud[] = {0x06, 0x00, 0x00, 0x09, 0x60};
    for (size_t r = 0; r < sizeof(k_class22) / sizeof(k_class22[0]); r++) {
        const class22_row_t *row = &k_class22[r];
        for (size_t a = 0; a < row->count; a++) {
            assert_int_equal(
                dcu_north_accept_axdr(NULL, MK_OAD(row->oi, row->attrs[a]), baud, sizeof(baud)),
                EDGE_SLAVE_SRC_DENIED);
        }
    }
}

static void test_methods_are_unadvertised(void **state) {
    (void)state;
    for (size_t r = 0; r < sizeof(k_class22) / sizeof(k_class22[0]); r++) {
        const class22_row_t *row = &k_class22[r];
        assert_int_equal(dcu_698_call(MK_OMD(row->oi, 1), NULL, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
        assert_int_equal(dcu_698_call(MK_OMD(row->oi, 3), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
}

static void test_oi_specific_methods_and_attrs_are_defined(void **state) {
    (void)state;
    assert_int_equal(dcu_698_call(MK_OMD(OI_CHANNEL_CONFIG, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_RS485_PORT, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_LOCAL_MODULE, 128), NULL, NULL),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_call(MK_OMD(OI_LOCAL_MODULE, 129), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(OI_DIGITAL_INPUT, 127), NULL, NULL),
                     EDGE_SLAVE_SRC_UNDEFINED);

    assert_false(dcu_698_class_defines_attr(IC_PERIPHERAL, 5));
    assert_true(dcu_698_oi_defines_attr(OI_LOCAL_MODULE, IC_PERIPHERAL, 6));
    assert_true(dcu_698_oi_defines_attr(OI_WIRELESS_PRIVATE, IC_PERIPHERAL, 5));
    assert_false(dcu_698_oi_defines_attr(OI_DIGITAL_INPUT, IC_PERIPHERAL, 5));
    assert_false(dcu_698_oi_defines_attr(OI_LOCAL_MODULE, IC_PERIPHERAL, 7));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_advertised_attrs_get_returns_dar2, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_unadvertised_attrs_dar_by_class_definition,
                                        setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_scalar_port_refuses_every_advertised_attr, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_advertised_attrs_are_read_only, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_methods_are_unadvertised, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_oi_specific_methods_and_attrs_are_defined, setup_model,
                                        teardown_model),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
