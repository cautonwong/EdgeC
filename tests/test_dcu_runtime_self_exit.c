#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>

#include "dcu_runtime_self_exit.h"

static void test_unset_is_zero(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms(NULL), 0);
}

static void test_five_seconds(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms("5"), 5000);
}

static void test_zero_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms("0"), 0);
}

static void test_invalid_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms("abc"), 0);
}

static void test_leading_space_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms(" 5"), 0);
}

static void test_plus_sign_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms("+5"), 0);
}

static void test_wrapped_negative_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms(" -18446744073709551615"), 0);
    assert_int_equal(dcu_runtime_self_exit_ms(" -18446744073709551614"), 0);
}

static void test_negative_is_disabled(void **state) {
    (void)state;
    assert_int_equal(dcu_runtime_self_exit_ms("-3"), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_unset_is_zero),
        cmocka_unit_test(test_five_seconds),
        cmocka_unit_test(test_zero_is_disabled),
        cmocka_unit_test(test_invalid_is_disabled),
        cmocka_unit_test(test_leading_space_is_disabled),
        cmocka_unit_test(test_plus_sign_is_disabled),
        cmocka_unit_test(test_wrapped_negative_is_disabled),
        cmocka_unit_test(test_negative_is_disabled),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
