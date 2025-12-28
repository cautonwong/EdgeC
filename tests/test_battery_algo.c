#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "battery_strategy.h"

static void test_battery_safety_low_soc(void **state) {
    (void)state;

    battery_state_t b_state = {.soc = 5.0f,  // 极低 SoC
                               .temperature = 25.0f,
                               .rated_capacity = 5000.0f};

    battery_limit_t b_limits = {.min_soc = 10.0f,  // 最小允许 10%
                                .max_soc = 95.0f,
                                .max_charge_pwr = 5000.0f,
                                .max_discharge_pwr = 5000.0f};

    // 模拟 JS 侧发出了 -3000W (放电) 的请求
    float target_pwr = -3000.0f;
    float safe_pwr = algo_battery_safety_clamp(&b_state, &b_limits, target_pwr);

    // 预期：因为 SoC 太低，放电被拒绝，功率修正为 0
    assert_float_equal(safe_pwr, 0.0f, 0.001f);
}

static void test_battery_safety_normal(void **state) {
    (void)state;

    battery_state_t b_state = {.soc = 50.0f,  // 正常 SoC
                               .temperature = 25.0f,
                               .rated_capacity = 5000.0f};

    battery_limit_t b_limits = {.min_soc = 10.0f,
                                .max_soc = 95.0f,
                                .max_charge_pwr = 5000.0f,
                                .max_discharge_pwr = 5000.0f};

    // 模拟 JS 发出 2000W 充电请求
    float target_pwr = 2000.0f;
    float safe_pwr = algo_battery_safety_clamp(&b_state, &b_limits, target_pwr);

    // 预期：正常执行
    assert_float_equal(safe_pwr, 2000.0f, 0.001f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_battery_safety_low_soc),
        cmocka_unit_test(test_battery_safety_normal),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
