#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_698_catalog_expected.h"
#include "dcu_698_object_model.h"

#define CATALOG_OI_COUNT (sizeof(dcu698_catalog_ois) / sizeof(dcu698_catalog_ois[0]))
#define CATALOG_OI_ATTR_COUNT (sizeof(dcu698_catalog_oi_attrs) / sizeof(dcu698_catalog_oi_attrs[0]))
#define CATALOG_CLASS_COUNT (sizeof(dcu698_catalog_classes) / sizeof(dcu698_catalog_classes[0]))

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

static void test_every_table_oi_is_in_the_catalog(void **state) {
    (void)state;
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
            if (dcu_mask_test(obj->attr_mask, (uint8_t)id)) assert_true(id <= max_attr);
            if (dcu_mask_test(obj->method_mask, (uint8_t)id)) assert_true(id <= cls->max_method);
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
    for (size_t i = 0; i < CATALOG_CLASS_COUNT; i++) {
        const dcu698_catalog_class_t *cls = &dcu698_catalog_classes[i];
        for (unsigned attr = 0; attr <= 255; attr++) {
            bool want = (attr >= 1 && attr <= cls->max_attr);
            assert_int_equal(dcu_698_class_defines_attr(cls->class_id, (uint8_t)attr), want);
        }
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_every_table_oi_is_in_the_catalog),
        cmocka_unit_test(test_masks_stay_within_the_class_definition),
        cmocka_unit_test(test_class_attr_limit_matches_catalog),
        cmocka_unit_test(test_no_duplicate_ois),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
