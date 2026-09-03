/**
 * @file test_smartbox_interlock_thermal.c
 * @brief Empirical Verification for SmartBox Transformer Thermal Model & Five-Safety Interlocks (AEP-442)
 */

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cmocka.h>

#include "smartbox_thermal_aging.h"
#include "smartbox_five_safety_interlock.h"

static void test_transformer_thermal_hotspot_and_life_loss(void **state) {
    (void)state;

    smartbox_thermal_model_t model;
    smartbox_thermal_init(&model, 630.0); // 630 kVA

    // 1. 额定满载基准测试 (Load=630kVA, K=1.0, TopOil=80℃, Ambient=30℃)
    // 最热点温升 = 30K -> θH = 80 + 30 = 110.0℃
    // 基准 110℃ 对应相对老化率 FAA 应严格为 1.000
    int rc = smartbox_thermal_evaluate_cycle(&model, 30.0, 80.0, 630.0, 3600.0);
    assert_int_equal(rc, 0);
    assert_float_equal(model.hotspot_temp_c, 110.0, 0.01);
    assert_float_equal(model.relative_aging_rate_faa, 1.0, 0.01);
    assert_false(model.is_thermal_alarm);
    // 运行 1 小时, 损耗 = 1 / 180000 * 100%
    assert_float_equal((float)model.cumulative_loss_of_life_pct, (float)((1.0 / 180000.0) * 100.0), 0.0001f);
    assert_float_equal((float)model.remaining_useful_life_hours, 179999.0f, 0.1f);

    // 2. 重载加速老化与高温报警测试 (Load=945kVA, K=1.5, TopOil=95℃, Ambient=35℃)
    // 绕组最热点温升加剧, θH > 140℃ 触发热过载报警
    rc = smartbox_thermal_evaluate_cycle(&model, 35.0, 95.0, 945.0, 3600.0);
    assert_int_equal(rc, 0);
    assert_true(model.hotspot_temp_c > 140.0);
    assert_true(model.relative_aging_rate_faa > 5.0); // 老化速度超过标称 5 倍以上
    assert_true(model.is_thermal_alarm);
}

static void test_five_safety_interlock_normal_operations(void **state) {
    (void)state;

    smartbox_switchgear_bay_t bay;
    smartbox_interlock_bay_init(&bay, 1);

    const char *reason = NULL;

    // 初始状态: 检修态 (全部断开，线路不带电，接地刀闸合闸，柜门关)
    bay.is_earth_switch_closed = true;
    bay.is_line_charged = false;
    bay.is_busbar_charged = false;

    // 1. 检修态下允许开柜门
    interlock_result_t res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_OPEN_CUBICLE_DOOR, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);

    // 2. 检修完毕关门，分接地刀闸
    bay.is_cubicle_door_open = false;
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_OPEN_EARTH_SWITCH, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);
    bay.is_earth_switch_closed = false;

    // 3. 断路器分位下，允许合母线刀闸和出线刀闸
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_BUS_ISOLATOR, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);
    bay.is_bus_isolator_closed = true;

    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_LINE_ISOLATOR, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);
    bay.is_line_isolator_closed = true;

    // 4. 接地刀闸已分且刀闸已合，允许合断路器送电
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_BREAKER, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);
    bay.is_breaker_closed = true;
    bay.is_line_charged = true;
}

static void test_five_safety_interlock_forbidden_violations(void **state) {
    (void)state;

    smartbox_switchgear_bay_t bay;
    smartbox_interlock_bay_init(&bay, 1);
    const char *reason = NULL;

    // 场景 1: 带负荷拉/合隔离开关 (断路器处于合位) -> 触发 Rule 2 闭锁
    bay.is_breaker_closed = true;
    bay.is_bus_isolator_closed = true;
    interlock_result_t res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_OPEN_BUS_ISOLATOR, &reason);
    assert_int_equal(res, INTERLOCK_BLOCK_LOADED_ISOLATOR);

    // 场景 2: 带电合接地刀闸 (主回路带电或开关合闸) -> 触发 Rule 3 闭锁
    bay.is_line_charged = true;
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_EARTH_SWITCH, &reason);
    assert_int_equal(res, INTERLOCK_BLOCK_CHARGED_EARTH);

    // 场景 3: 带接地线合断路器/隔离开关 (接地刀闸合闸) -> 触发 Rule 4 闭锁
    bay.is_breaker_closed = false;
    bay.is_earth_switch_closed = true;
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_BREAKER, &reason);
    assert_int_equal(res, INTERLOCK_BLOCK_EARTHED_CLOSING);

    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_CLOSE_BUS_ISOLATOR, &reason);
    assert_int_equal(res, INTERLOCK_BLOCK_EARTHED_CLOSING);

    // 场景 4: 误入带电间隔 (回路带电且未可靠接地时开门) -> 触发 Rule 5 闭锁
    bay.is_line_charged = true;
    bay.is_earth_switch_closed = false;
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_OPEN_CUBICLE_DOOR, &reason);
    assert_int_equal(res, INTERLOCK_BLOCK_CHARGED_DOOR_ENTRY);

    // 场景 5: 强制万能解锁钥匙使能 -> 绕过闭锁
    bay.is_lock_override_key_active = true;
    res = smartbox_interlock_evaluate(&bay, INTERLOCK_OP_OPEN_CUBICLE_DOOR, &reason);
    assert_int_equal(res, INTERLOCK_PERMITTED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_transformer_thermal_hotspot_and_life_loss),
        cmocka_unit_test(test_five_safety_interlock_normal_operations),
        cmocka_unit_test(test_five_safety_interlock_forbidden_violations),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
