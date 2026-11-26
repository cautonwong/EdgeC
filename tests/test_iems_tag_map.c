#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_driver_adapter.h"
#include "iems_point_model.h"

typedef struct {
    const char *name;
    uint32_t tag_id;
    uint32_t dense;
    int slot_at_device_8191;
} mapped_tag_t;

static const mapped_tag_t k_mapped[] = {
    {"VOLTAGE_PHASE_A", 1001, 0, 131056},   {"CURRENT_PHASE_A", 1002, 1, 131057},
    {"GRID_POWER", 1003, 2, 131058},        {"GRID_FREQUENCY", 1004, 3, 131059},
    {"ADC_RAW_DATA", 2001, 4, 131060},      {"ADC_VOLTAGE_RMS", 2002, 5, 131061},
    {"ENERGY_ACTIVE_IMP", 3001, 6, 131062}, {"BATTERY_SOC", 4001, 7, 131063},
    {"BATTERY_CURRENT", 4002, 8, 131064},   {"BATTERY_POWER", 4003, 9, 131065},
    {"SOLAR_POWER", 8001, 10, 131066},      {"SOLAR_VOLTS", 8003, 11, 131067},
    {"SOLAR_RELAY", 8004, 12, 131068},      {"PCS_CONTROL", 8005, 13, 131069},
};

static void test_short_names_share_dense_with_point_model(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_mapped) / sizeof(k_mapped[0]); i++) {
        uint32_t tag = 0;
        assert_int_equal(iems_point_lookup_name(k_mapped[i].name, &tag), 0);
        assert_int_equal(tag, k_mapped[i].tag_id);

        uint32_t dense = 0xA5A5A5A5u;
        assert_int_equal(iems_point_slot(tag, &dense), 0);
        assert_int_equal(dense, k_mapped[i].dense);
        assert_int_equal(iems_tag_map_fn(tag, 0), (int)k_mapped[i].dense);
    }
}

static void test_device_8191_uses_product_slot_math(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_mapped) / sizeof(k_mapped[0]); i++) {
        assert_int_equal(iems_tag_map_fn(k_mapped[i].tag_id, 8191),
                         k_mapped[i].slot_at_device_8191);
    }
}

static void test_legacy_names_stay_unmapped(void **state) {
    (void)state;
    static const char *k_legacy[] = {
        "LC1.PCS.SmokeDetectStatus", "EMS.EMS.DcTotalVolUnusual",
        "EMS.ActivePower",           "EMS.SetConnect",
        "PCS.AphaseApparentPower",   "PCS_METER.AphaseActivePower",
        "EMS.EnIoProtect",           "pcs456.constant vol_chg_vol_setting",
    };
    for (size_t i = 0; i < sizeof(k_legacy) / sizeof(k_legacy[0]); i++) {
        uint32_t tag = 0xA5A5A5A5u;
        assert_int_equal(iems_point_lookup_name(k_legacy[i], &tag), -1);
        assert_int_equal(tag, 0xA5A5A5A5u);
    }
}

static void test_unknown_tag_below_64_falls_back_to_tag_id(void **state) {
    (void)state;
    assert_int_equal(iems_tag_map_fn(0, 5), 0);
    assert_int_equal(iems_tag_map_fn(37, 5), 37);
    assert_int_equal(iems_tag_map_fn(63, 8191), 63);
    assert_int_equal(iems_tag_map_fn(64, 5), -1);
    assert_int_equal(iems_tag_map_fn(1005, 5), -1);
    assert_int_equal(iems_tag_map_fn(9999, 5), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_short_names_share_dense_with_point_model),
        cmocka_unit_test(test_device_8191_uses_product_slot_math),
        cmocka_unit_test(test_legacy_names_stay_unmapped),
        cmocka_unit_test(test_unknown_tag_below_64_falls_back_to_tag_id),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
