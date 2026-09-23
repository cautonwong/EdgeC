/**
 * @file test_device_twin.c
 * @brief Unit tests for State-Reconciled Device Twin Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device_twin.h"

static void test_device_twin_init_null(void **state) {
    (void)state;
    device_twin_t twin;
    assert_int_equal(device_twin_init(NULL, 101), -1);
    assert_int_equal(device_twin_init(&twin, 0), -1);
}

static void test_device_twin_workflow(void **state) {
    (void)state;
    device_twin_t twin;
    assert_int_equal(device_twin_init(&twin, 1001), 0);
    assert_int_equal(twin.device_id, 1001);

    /* Set Desired Target Speed = 1500.0 RPM */
    assert_int_equal(device_twin_set_desired(&twin, 10, 1500.0), 0);

    /* Reconcile -> Unsynced = 1 (Reported is 0.0) */
    size_t unsynced = 0;
    assert_int_equal(device_twin_reconcile(&twin, &unsynced), 0);
    assert_int_equal(unsynced, 1);

    /* Set Reported Speed = 1500.0 RPM */
    assert_int_equal(device_twin_set_reported(&twin, 10, 1500.0), 0);

    /* Reconcile -> Unsynced = 0 (State Reconciled) */
    assert_int_equal(device_twin_reconcile(&twin, &unsynced), 0);
    assert_int_equal(unsynced, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_device_twin_init_null),
        cmocka_unit_test(test_device_twin_workflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
