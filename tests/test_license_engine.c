#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "edge_license.h"

static void test_license_basic_tier(void** state) {
    (void)state;
    // Test basic tier validation
    int rc = license_engine_init("BASIC-KEY-123");
    // Since CMake currently builds with EDGEC_TIER_ENTERPRISE by default,
    // a BASIC key should FAIL validation for an ENTERPRISE binary.
    assert_int_equal(rc, -1);
}

static void test_license_enterprise_tier(void** state) {
    (void)state;
    // Test enterprise tier validation
    int rc = license_engine_init("ENT-EVAL-KEY");
    assert_int_equal(rc, 0);

    // Enterprise key on Enterprise binary should support everything
    assert_true(license_engine_check_feature("WASM"));
    assert_true(license_engine_check_feature("DSP"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_license_basic_tier),
        cmocka_unit_test(test_license_enterprise_tier),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
