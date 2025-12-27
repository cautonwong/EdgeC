#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "edge_version.h"

static void test_addition(void **state) {
    (void)state; /* unused */
    assert_int_equal(edge_add(2, 3), 5);
    assert_int_equal(edge_add(-1, 1), 0);
}

static void test_version_string(void **state) {
    (void)state;
    const char *v = edge_get_version();
    assert_non_null(v);
    assert_string_equal(v, "EdgeOS v0.1.0 (Alpha)");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_version_string),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
