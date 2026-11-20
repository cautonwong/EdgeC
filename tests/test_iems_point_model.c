#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "iems_point_model.h"

typedef struct {
    const char *name;
    uint32_t tag_id;
    uint32_t slot;
} known_point_t;

/* 期望值来自 iems_driver_adapter.c 的 14 条元数据与 iems_tag_map_fn 静态表，
 * 不从被测表反推。 */
static const known_point_t k_known[] = {
    {"VOLTAGE_PHASE_A", 1001, 0},   {"CURRENT_PHASE_A", 1002, 1}, {"GRID_POWER", 1003, 2},
    {"GRID_FREQUENCY", 1004, 3},    {"ADC_RAW_DATA", 2001, 4},    {"ADC_VOLTAGE_RMS", 2002, 5},
    {"ENERGY_ACTIVE_IMP", 3001, 6}, {"BATTERY_SOC", 4001, 7},     {"BATTERY_CURRENT", 4002, 8},
    {"BATTERY_POWER", 4003, 9},     {"SOLAR_POWER", 8001, 10},    {"SOLAR_VOLTS", 8003, 11},
    {"SOLAR_RELAY", 8004, 12},      {"PCS_CONTROL", 8005, 13},
};

/* tag-config-map.md 里点名的真实旧名；没有模板/点表输入，当前不可映射。 */
static const char *k_unmapped_legacy[] = {
    "LC1.PCS.SmokeDetectStatus", "EMS.EMS.DcTotalVolUnusual",
    "EMS.ActivePower",           "EMS.SetConnect",
    "PCS.AphaseApparentPower",   "PCS_METER.AphaseActivePower",
    "EMS.EnIoProtect",           "pcs456.constant vol_chg_vol_setting",
};

static void test_known_name_round_trip(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_known) / sizeof(k_known[0]); i++) {
        uint32_t tag = 0xA5A5A5A5u;
        assert_int_equal(iems_point_lookup_name(k_known[i].name, &tag), 0);
        assert_int_equal(tag, k_known[i].tag_id);

        const char *name = NULL;
        assert_int_equal(iems_point_lookup_tag(k_known[i].tag_id, &name), 0);
        assert_string_equal(name, k_known[i].name);

        uint32_t slot = 0xA5A5A5A5u;
        assert_int_equal(iems_point_slot(k_known[i].tag_id, &slot), 0);
        assert_int_equal(slot, k_known[i].slot);
    }
}

static void test_unknown_name_and_tag_do_not_write(void **state) {
    (void)state;
    const char *sentinel = "sentinel";

    uint32_t tag = 0xA5A5A5A5u;
    assert_int_equal(iems_point_lookup_name("EMS.NotARealPoint", &tag), -1);
    assert_int_equal(tag, 0xA5A5A5A5u);

    const char *name = sentinel;
    assert_int_equal(iems_point_lookup_tag(9999, &name), -1);
    assert_ptr_equal(name, sentinel);

    uint32_t slot = 0xA5A5A5A5u;
    assert_int_equal(iems_point_slot(9999, &slot), -1);
    assert_int_equal(slot, 0xA5A5A5A5u);

    assert_int_equal(iems_point_lookup_name(NULL, &tag), -1);
    assert_int_equal(iems_point_lookup_name("VOLTAGE_PHASE_A", NULL), -1);
    assert_int_equal(iems_point_lookup_tag(1001, NULL), -1);
    assert_int_equal(iems_point_slot(1001, NULL), -1);
    assert_int_equal(iems_point_slot(IEMS_POINT_TAG_NONE, &slot), -1);
    assert_int_equal(slot, 0xA5A5A5A5u);
}

static void expect_slot(uint16_t device_id, uint16_t dense_idx, int want_rc, uint32_t want_slot) {
    iems_point_key_t key = {.device_id = device_id, .dense_idx = dense_idx};
    uint32_t slot = 0xA5A5A5A5u;
    assert_int_equal(iems_point_key_slot(&key, &slot), want_rc);
    if (want_rc == 0) {
        assert_int_equal(slot, want_slot);
    } else {
        assert_int_equal(slot, 0xA5A5A5A5u);
    }
}

static void test_slot_boundaries(void **state) {
    (void)state;
    expect_slot(0, 0, 0, 0);
    expect_slot(0, 15, 0, 15);
    expect_slot(1, 0, 0, 16);
    expect_slot(1, 15, 0, 31);
    expect_slot(63, 0, 0, 1008);
    expect_slot(63, 15, 0, 1023);
    expect_slot(64, 0, -1, 0);
    expect_slot(0, 16, -1, 0);
    expect_slot(64, 16, -1, 0);
    expect_slot(65535, 65535, -1, 0);

    iems_point_key_t key = {.device_id = 0, .dense_idx = 0};
    uint32_t slot = 0xA5A5A5A5u;
    assert_int_equal(iems_point_key_slot(NULL, &slot), -1);
    assert_int_equal(iems_point_key_slot(&key, NULL), -1);
    assert_int_equal(slot, 0xA5A5A5A5u);
}

/* 重名由表结构与测试共同排除：名字两两不同，已映射 tag 两两不同。 */
static void test_names_and_mapped_tags_are_unique(void **state) {
    (void)state;
    assert_true(g_iems_point_names_len > 0);
    for (size_t i = 0; i < g_iems_point_names_len; i++) {
        assert_non_null(g_iems_point_names[i].name);
        for (size_t j = i + 1; j < g_iems_point_names_len; j++) {
            assert_true(strcmp(g_iems_point_names[i].name, g_iems_point_names[j].name) != 0);
            uint32_t a = g_iems_point_names[i].tag_id;
            uint32_t b = g_iems_point_names[j].tag_id;
            if (a != IEMS_POINT_TAG_NONE && b != IEMS_POINT_TAG_NONE) {
                assert_true(a != b);
            }
        }
    }
}

static void test_legacy_samples_are_listed_but_unmapped(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_unmapped_legacy) / sizeof(k_unmapped_legacy[0]); i++) {
        const char *name = k_unmapped_legacy[i];
        size_t j = 0;
        for (; j < g_iems_point_names_len; j++) {
            if (strcmp(g_iems_point_names[j].name, name) == 0) break;
        }
        assert_true(j < g_iems_point_names_len);
        assert_int_equal(g_iems_point_names[j].tag_id, IEMS_POINT_TAG_NONE);

        uint32_t tag = 0xA5A5A5A5u;
        assert_int_equal(iems_point_lookup_name(name, &tag), -1);
        assert_int_equal(tag, 0xA5A5A5A5u);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_known_name_round_trip),
        cmocka_unit_test(test_unknown_name_and_tag_do_not_write),
        cmocka_unit_test(test_slot_boundaries),
        cmocka_unit_test(test_names_and_mapped_tags_are_unique),
        cmocka_unit_test(test_legacy_samples_are_listed_but_unmapped),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
