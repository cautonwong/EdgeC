#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "edge_version.h"

static void test_addition(void** state) {
    (void)state;
    assert_int_equal(1 + 1, 2);
}

static void test_version_structure(void** state) {
    (void)state;
    const edge_version_t* v = edge_get_version();
    // [高质量对齐] 验证主版本号是否为 V4.x
    assert_int_equal(v->major, 4);
    assert_non_null(v->version_str);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_version_structure),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
