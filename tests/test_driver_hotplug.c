/**
 * @file test_driver_hotplug.c
 * @brief Unit tests for Dynamic Driver Hot-Plug & ABI Guard Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver_hotplug.h"

static int dummy_init_flag = 0;
static int dummy_driver_init(void) {
    dummy_init_flag = 1;
    return 0;
}

static void dummy_driver_cleanup(void) {
    dummy_init_flag = 0;
}

static void test_driver_hotplug_init_null(void **state) {
    (void)state;
    assert_int_equal(driver_hotplug_init(NULL), -1);
}

static void test_driver_hotplug_abi_validation(void **state) {
    (void)state;
    driver_hotplug_mgr_storage_t store;
    driver_hotplug_mgr_t *mgr = driver_hotplug_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_int_equal(driver_hotplug_init(mgr), 0);

    driver_plugin_ops_t bad_ops = {.abi_magic = 0xDEADBEEF,
                                   .abi_version = DRIVER_ABI_VERSION,
                                   .driver_name = "BadAbiDriver",
                                   .init_fn = dummy_driver_init,
                                   .cleanup_fn = dummy_driver_cleanup};

    uint32_t slot_id = 0;
    assert_int_equal(driver_hotplug_load(mgr, "/lib/bad_driver.so", &bad_ops, &slot_id), -2);

    driver_plugin_ops_t good_ops = {.abi_magic = DRIVER_ABI_MAGIC,
                                    .abi_version = DRIVER_ABI_VERSION,
                                    .driver_name = "GoodDriver",
                                    .init_fn = dummy_driver_init,
                                    .cleanup_fn = dummy_driver_cleanup};

    assert_int_equal(driver_hotplug_load(mgr, "/lib/good_driver.so", &good_ops, &slot_id), 0);
    assert_int_equal(slot_id, 1);
    assert_int_equal(dummy_init_flag, 1);
    assert_int_equal(driver_hotplug_get_active_count(mgr), 1);

    assert_int_equal(driver_hotplug_unload(mgr, slot_id), 0);
    assert_int_equal(dummy_init_flag, 0);
    assert_int_equal(driver_hotplug_get_active_count(mgr), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_driver_hotplug_init_null),
        cmocka_unit_test(test_driver_hotplug_abi_validation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
