#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_catalog_expected.h"
#include "dcu_698_object_internal.h"
#include "dcu_698_object_model.h"
#include "dcu_discovery.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

#define CATALOG_OI_COUNT (sizeof(dcu698_catalog_ois) / sizeof(dcu698_catalog_ois[0]))
#define CATALOG_OI_ATTR_COUNT (sizeof(dcu698_catalog_oi_attrs) / sizeof(dcu698_catalog_oi_attrs[0]))
#define CATALOG_CLASS_COUNT (sizeof(dcu698_catalog_classes) / sizeof(dcu698_catalog_classes[0]))
#define CATALOG_OI_METHOD_COUNT \
    (sizeof(dcu698_catalog_oi_methods) / sizeof(dcu698_catalog_oi_methods[0]))

static const dcu698_catalog_oi_t *catalog_oi(uint16_t oi) {
    for (size_t i = 0; i < CATALOG_OI_COUNT; i++) {
        if (dcu698_catalog_ois[i].oi == oi) return &dcu698_catalog_ois[i];
    }
    return NULL;
}

static uint8_t catalog_oi_max_attr(uint16_t oi) {
    for (size_t i = 0; i < CATALOG_OI_ATTR_COUNT; i++) {
        if (dcu698_catalog_oi_attrs[i].oi == oi) return dcu698_catalog_oi_attrs[i].max_attr;
    }
    return 0;
}

static const dcu698_catalog_class_t *catalog_class(uint8_t class_id) {
    for (size_t i = 0; i < CATALOG_CLASS_COUNT; i++) {
        if (dcu698_catalog_classes[i].class_id == class_id) return &dcu698_catalog_classes[i];
    }
    return NULL;
}

static bool catalog_oi_defines_method(uint16_t oi, uint8_t method) {
    for (size_t i = 0; i < CATALOG_OI_METHOD_COUNT; i++) {
        if (dcu698_catalog_oi_methods[i].oi == oi &&
            dcu698_catalog_oi_methods[i].method == method) {
            return true;
        }
    }
    return false;
}

static bool class_has_absent_method(uint8_t class_id, uint8_t method_id) {
    if ((class_id == 7 || class_id == 24) && method_id == 3) return true;
    if (class_id == 9 && method_id == 6) return true;
    return false;
}

static void init_model(dcu_inventory_t *inv, dcu_poller_t *poller) {
    assert_int_equal(dcu_inventory_init(inv), 0);
    assert_int_equal(dcu_poller_init(poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(inv, poller, sink, NULL, NULL, NULL, NULL, bus), 0);
}

static void test_every_table_oi_is_in_the_catalog(void **state) {
    (void)state;
    assert_int_equal(dcu_698_object_count(), 136);
    for (size_t i = 0; i < dcu_698_object_count(); i++) {
        const dcu_698_object_t *obj = dcu_698_object_at(i);
        assert_non_null(obj);
        const dcu698_catalog_oi_t *row = catalog_oi(obj->oi);
        assert_non_null(row);
        assert_int_equal(obj->class_id, row->class_id);
    }
}

static void test_masks_stay_within_the_class_definition(void **state) {
    (void)state;
    for (size_t i = 0; i < dcu_698_object_count(); i++) {
        const dcu_698_object_t *obj = dcu_698_object_at(i);
        const dcu698_catalog_class_t *cls = catalog_class(obj->class_id);
        assert_non_null(cls);
        /* 附录 A 的实例属性可以超出 7.3 类表（6002 的 A6/A10）。 */
        uint8_t oi_max = catalog_oi_max_attr(obj->oi);
        uint8_t max_attr = oi_max > cls->max_attr ? oi_max : cls->max_attr;
        for (unsigned id = 0; id <= 255; id++) {
            if (dcu_mask_test(obj->attr_mask, (uint8_t)id)) {
                assert_true(id >= 1 && id <= max_attr);
            }
            if (dcu_mask_test(obj->method_mask, (uint8_t)id)) {
                assert_true(id >= 1 && id <= cls->max_method);
            }
        }
    }
}

static void test_methods_stay_within_the_oi_definition(void **state) {
    (void)state;
    for (size_t i = 0; i < dcu_698_object_count(); i++) {
        const dcu_698_object_t *obj = dcu_698_object_at(i);
        const dcu698_catalog_class_t *cls = catalog_class(obj->class_id);
        assert_non_null(cls);
        for (unsigned id = 0; id <= 255; id++) {
            if (!dcu_mask_test(obj->method_mask, (uint8_t)id)) continue;
            assert_true(id >= 1);
            if (id <= cls->max_class_method) {
                assert_true(dcu_698_class_defines_method(obj->class_id, (uint8_t)id));
                continue;
            }
            assert_true(catalog_oi_defines_method(obj->oi, (uint8_t)id));
        }
    }
}

static void test_no_duplicate_ois(void **state) {
    (void)state;
    const size_t count = dcu_698_object_count();
    for (size_t i = 0; i < count; i++) {
        const dcu_698_object_t *a = dcu_698_object_at(i);
        for (size_t j = i + 1; j < count; j++) {
            const dcu_698_object_t *b = dcu_698_object_at(j);
            assert_int_not_equal(a->oi, b->oi);
        }
    }
}

static void test_class_attr_limit_matches_catalog(void **state) {
    (void)state;
    assert_false(dcu_698_class_defines_attr(0, 1));
    assert_false(dcu_698_class_defines_attr(27, 1));
    for (size_t i = 0; i < CATALOG_CLASS_COUNT; i++) {
        const dcu698_catalog_class_t *cls = &dcu698_catalog_classes[i];
        for (unsigned attr = 0; attr <= 255; attr++) {
            bool want = (attr >= 1 && attr <= cls->max_attr);
            assert_int_equal(dcu_698_class_defines_attr(cls->class_id, (uint8_t)attr), want);
        }
    }
}

static void test_class_method_limit_matches_catalog(void **state) {
    (void)state;
    assert_false(dcu_698_class_defines_method(0, 1));
    assert_false(dcu_698_class_defines_method(27, 1));
    for (size_t i = 0; i < CATALOG_CLASS_COUNT; i++) {
        const dcu698_catalog_class_t *cls = &dcu698_catalog_classes[i];
        for (unsigned m = 0; m <= 255; m++) {
            bool want = (m >= 1 && m <= cls->max_class_method &&
                         !class_has_absent_method(cls->class_id, (uint8_t)m));
            assert_int_equal(dcu_698_class_defines_method(cls->class_id, (uint8_t)m), want);
        }
    }
}

static uint8_t read_u8(const uint8_t *buf, size_t len, size_t *off) {
    assert_true(*off < len);
    return buf[(*off)++];
}

static size_t read_axdr_len(const uint8_t *buf, size_t len, size_t *off) {
    uint8_t b = read_u8(buf, len, off);
    if ((b & 0x80u) == 0) return b;
    uint8_t n = (uint8_t)(b & 0x7Fu);
    assert_true(n >= 1 && n <= sizeof(size_t));
    size_t val = 0;
    for (uint8_t i = 0; i < n; i++) {
        val = (val << 8) | read_u8(buf, len, off);
    }
    return val;
}

static void test_appconn_acl_matches_object_model_and_catalog(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    uint8_t buf[16384];
    size_t len = 0;
    assert_int_equal(dcu_698_appconn_object_list(buf, 10, &len), -1);
    assert_int_equal(dcu_698_appconn_resolve(NULL, 0x44000300u, buf, sizeof(buf), &len), -1);
    assert_int_equal(dcu_698_appconn_resolve(NULL, 0x44000200u, buf, sizeof(buf), &len), 0);

    size_t off = 0;
    assert_int_equal(read_u8(buf, len, &off), 0x01); /* array */
    size_t obj_count = read_axdr_len(buf, len, &off);
    assert_int_equal(obj_count, 131);

    uint16_t seen_ois[131];
    for (size_t i = 0; i < obj_count; i++) {
        assert_int_equal(read_u8(buf, len, &off), 0x02); /* structure(2) */
        assert_int_equal(read_u8(buf, len, &off), 0x02);
        assert_int_equal(read_u8(buf, len, &off), 0x50); /* OI */
        uint8_t hi = read_u8(buf, len, &off);
        uint8_t lo = read_u8(buf, len, &off);
        uint16_t oi = (uint16_t)(((uint16_t)hi << 8) | lo);

        for (size_t j = 0; j < i; j++) {
            assert_int_not_equal(seen_ois[j], oi);
        }
        seen_ois[i] = oi;

        const dcu_698_object_t *obj = dcu_698_find(oi);
        assert_non_null(obj);
        const dcu698_catalog_oi_t *row = catalog_oi(oi);
        assert_non_null(row);
        assert_int_equal(obj->class_id, row->class_id);
        const dcu698_catalog_class_t *cls = catalog_class(obj->class_id);
        assert_non_null(cls);

        uint8_t oi_max = catalog_oi_max_attr(oi);
        uint8_t max_attr = oi_max > cls->max_attr ? oi_max : cls->max_attr;

        assert_int_equal(read_u8(buf, len, &off), 0x02); /* structure(2) */
        assert_int_equal(read_u8(buf, len, &off), 0x02);
        assert_int_equal(read_u8(buf, len, &off), 0x01); /* attrs array */
        size_t n_attrs = read_axdr_len(buf, len, &off);
        for (size_t a = 0; a < n_attrs; a++) {
            assert_int_equal(read_u8(buf, len, &off), 0x02);
            assert_int_equal(read_u8(buf, len, &off), 0x02);
            assert_int_equal(read_u8(buf, len, &off), 0x11); /* unsigned */
            uint8_t attr_id = read_u8(buf, len, &off);
            assert_int_equal(read_u8(buf, len, &off), 0x16); /* enum */
            uint8_t rights = read_u8(buf, len, &off);

            assert_true(rights <= 3);
            assert_true(attr_id >= 1 && attr_id <= max_attr);
        }

        assert_int_equal(read_u8(buf, len, &off), 0x01); /* methods array */
        size_t n_methods = read_axdr_len(buf, len, &off);
        for (size_t m = 0; m < n_methods; m++) {
            assert_int_equal(read_u8(buf, len, &off), 0x02);
            assert_int_equal(read_u8(buf, len, &off), 0x02);
            assert_int_equal(read_u8(buf, len, &off), 0x11); /* unsigned */
            uint8_t method_id = read_u8(buf, len, &off);
            assert_int_equal(read_u8(buf, len, &off), 0x03); /* bool */
            uint8_t exec = read_u8(buf, len, &off);

            assert_int_equal(exec, 1);
            assert_true(method_id >= 1 && method_id <= cls->max_method);
            if (method_id <= cls->max_class_method) {
                assert_true(dcu_698_class_defines_method(obj->class_id, method_id));
            } else {
                assert_true(catalog_oi_defines_method(oi, method_id));
            }
        }
    }
    assert_int_equal(off, len);
    dcu_698_object_model_reset();
}

static void test_negative_unregistered_oi_returns_not_found(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    static const uint16_t k_unregistered[] = {0x0001, 0x1999, 0x9999, 0xDEAD};
    const uint8_t axdr_u8[] = {0x11, 0x01};
    uint8_t buf[128];
    size_t len = 0;

    for (size_t i = 0; i < sizeof(k_unregistered) / sizeof(k_unregistered[0]); i++) {
        uint16_t oi = k_unregistered[i];
        const dcu_698_object_t *obj_out = NULL;
        assert_null(dcu_698_find(oi));
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(oi, 2), buf, sizeof(buf), &len),
                         EDGE_SLAVE_SRC_NOT_FOUND);
        assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(oi, 2), axdr_u8, sizeof(axdr_u8)),
                         EDGE_SLAVE_SRC_NOT_FOUND);
        assert_int_equal(dcu_698_check_call(MK_OMD(oi, 1), &obj_out), EDGE_SLAVE_SRC_NOT_FOUND);
        assert_null(obj_out);
        assert_int_equal(dcu_698_call(MK_OMD(oi, 1), NULL, NULL), EDGE_SLAVE_SRC_NOT_FOUND);
    }
    dcu_698_object_model_reset();
}

static void test_negative_out_of_class_attr_returns_undefined(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_discovery_t disc;
    dcu_event_obj_t evt_3000 = {0};
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    dcu_discovery_init(&disc);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&inv, &poller, sink, NULL, &disc, &evt_3000, NULL, bus), 0);

    static const struct {
        uint16_t oi;
        uint8_t bad_attr;
        bool check_resolve;
    } k_cases[] = {
        {0x0010, 6, false}, /* Class 1: 1..5 (var_encode_axdr base returns NOT_FOUND on GET) */
        {0x2000, 4, false}, /* Class 3: 1..3 (var_encode_axdr base returns NOT_FOUND on GET) */
        {0x3000, 15, true}, /* Class 24: 1..14 */
        {0x4000, 5, true},  /* Class 8 + A.4: 1..4 */
        {0x5000, 4, true},  /* Class 9: 1..3 */
        {0x6000, 4, true},  /* Class 10: 1..3 */
        {0x6002, 11, true}, /* Class 11 + A.7: 1..10 */
        {0x8000, 6, true},  /* Class 13: 1..5 */
        {0x4510, 6, true},  /* Class 26: 1..5 */
        {0xF200, 5, true},  /* Class 22: 1..4 */
        {0x2000, 0, false}, /* attr 0 is always undefined */
    };
    const uint8_t axdr_u8[] = {0x11, 0x01};
    uint8_t buf[128];
    size_t len = 0;

    for (size_t i = 0; i < sizeof(k_cases) / sizeof(k_cases[0]); i++) {
        uint32_t oad = MK_OAD(k_cases[i].oi, k_cases[i].bad_attr);
        if (k_cases[i].check_resolve) {
            assert_int_equal(dcu_north_resolve_axdr(NULL, oad, buf, sizeof(buf), &len),
                             EDGE_SLAVE_SRC_UNDEFINED);
        }
        assert_int_equal(dcu_north_accept_axdr(NULL, oad, axdr_u8, sizeof(axdr_u8)),
                         EDGE_SLAVE_SRC_UNDEFINED);
    }
    dcu_698_object_model_reset();
    dcu_discovery_destroy(&disc);
}

static void test_negative_out_of_class_method_returns_undefined(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    init_model(&inv, &poller);

    static const struct {
        uint16_t oi;
        uint8_t bad_method;
    } k_cases[] = {
        {0x3000, 0},   /* method 0 is always undefined */
        {0x3000, 3},   /* Class 24 absent method 3 */
        {0x3000, 6},   /* Class 24: 1..5 */
        {0x4300, 127}, /* 127..129 belong to 4307..4309, not 4300 */
        {0x4510, 2},   /* Class 26: 1 */
        {0x6000, 4},   /* Class 10: 1..3 + 127..134 */
        {0x6000, 135}, /* Class 10: max 134 */
        {0x6012, 131}, /* 6012: 1..3 + 127..130 */
        {0x8000, 8},   /* Class 13: 1..7 + 127..131 */
        {0x8000, 132}, /* 8000: max 131 */
        {0xF001, 11},  /* F001: 1..10 */
    };

    for (size_t i = 0; i < sizeof(k_cases) / sizeof(k_cases[0]); i++) {
        uint32_t omd = MK_OMD(k_cases[i].oi, k_cases[i].bad_method);
        const dcu_698_object_t *obj_out = NULL;
        assert_int_equal(dcu_698_check_call(omd, &obj_out), EDGE_SLAVE_SRC_UNDEFINED);
        assert_null(obj_out);
        assert_int_equal(dcu_698_call(omd, NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
    dcu_698_object_model_reset();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_every_table_oi_is_in_the_catalog),
        cmocka_unit_test(test_masks_stay_within_the_class_definition),
        cmocka_unit_test(test_methods_stay_within_the_oi_definition),
        cmocka_unit_test(test_no_duplicate_ois),
        cmocka_unit_test(test_class_attr_limit_matches_catalog),
        cmocka_unit_test(test_class_method_limit_matches_catalog),
        cmocka_unit_test(test_appconn_acl_matches_object_model_and_catalog),
        cmocka_unit_test(test_negative_unregistered_oi_returns_not_found),
        cmocka_unit_test(test_negative_out_of_class_attr_returns_undefined),
        cmocka_unit_test(test_negative_out_of_class_method_returns_undefined),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
