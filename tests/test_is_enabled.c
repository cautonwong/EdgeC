#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "contract_data_types.h"

// 模拟配置宏
#define TEST_CONFIG_ENABLED 1
#define TEST_CONFIG_DISABLED 0
// TEST_CONFIG_UNDEFINED 故意不定义

/**
 * 验证 IS_ENABLED 宏在各种配置情况下的表现
 */
static void test_is_enabled_logic(void **state) {
    (void)state;

    // 1. 验证定义为 1 的情况
    assert_true(IS_ENABLED(TEST_CONFIG_ENABLED));

    // 2. 验证定义为 0 的情况
    assert_false(IS_ENABLED(TEST_CONFIG_DISABLED));

    // 3. 验证未定义的情况 (这是宏魔法的核心：未定义应视为 0)
    // 注意：由于 C 语言限制，我们无法直接测试完全未定义的标识符作为参数
    // 但我们可以测试宏魔法在拼接后的回退逻辑
    assert_false(IS_ENABLED(TEST_CONFIG_UNDEFINED));
}

/**
 * 验证 IS_ENABLED 在运行时逻辑分支中的应用
 */
static void test_is_enabled_in_branch(void **state) {
    (void)state;
    int check = 0;

    if (IS_ENABLED(TEST_CONFIG_ENABLED)) {
        check = 100;
    } else {
        check = 200;
    }
    assert_int_equal(check, 100);

    if (IS_ENABLED(TEST_CONFIG_UNDEFINED)) {
        check = 300;
    } else {
        check = 400;
    }
    assert_int_equal(check, 400);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_is_enabled_logic),
        cmocka_unit_test(test_is_enabled_in_branch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
