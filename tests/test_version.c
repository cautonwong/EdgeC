#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "edge_version.h"

// 显式声明 edge_add (因为它是为了测试而存在的内部辅助函数)
extern int edge_add(int a, int b);

static void test_addition(void **state) {
    (void)state;
    assert_int_equal(edge_add(2, 3), 5);
    assert_int_equal(edge_add(-1, 1), 0);
}

static void test_version_structure(void **state) {
    (void)state;
    const edge_version_t *v = edge_get_version();

    assert_non_null(v);
    assert_int_equal(v->major, 4);
    assert_int_equal(v->minor, 5);
    assert_non_null(v->version_str);
    assert_non_null(v->git_hash);
    assert_non_null(v->build_time);

    // 验证版本字符串的前缀
    assert_true(strncmp(v->version_str, "4.5", 3) == 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addition),
        cmocka_unit_test(test_version_structure),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}