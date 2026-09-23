/**
 * @file test_iems_energy_strategy.c
 * @brief Empirical Verification for IEMS Comprehensive Energy Strategy: Demand Control, TOU &
 * Anti-Reverse (AEP-440)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iems_microgrid_coordinator.h"

static void test_tou_schedule_arbitrage(void **state) {
    (void)state;

    iems_microgrid_coordinator_t coord;
    iems_coordinator_init(&coord, 630.0, 500.0);

    iems_tou_planner_t planner;
    iems_coordinator_setup_default_tou(&planner);

    // 1. 谷段充电测试 (02:00, 谷时充电 -100kW, SOC=50%)
    int rc = iems_coordinator_evaluate_cycle(&coord, 150.0, 0.0, 50.0, 0.0, 50.0, 2, 0, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, -100.0, 0.01);
    assert_false(coord.is_anti_reverse_active);
    assert_false(coord.is_demand_shaving_active);

    // 2. 峰段放电测试 (09:30, 峰时放电 +100kW, SOC=80%)
    rc = iems_coordinator_evaluate_cycle(&coord, 300.0, 50.0, 350.0, 0.0, 80.0, 9, 30, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, 100.0, 0.01);

    // 3. 午间平段光伏吸收充电 (12:30, 平时充电 -80kW, SOC=60%)
    rc = iems_coordinator_evaluate_cycle(&coord, 100.0, 200.0, 150.0, 0.0, 60.0, 12, 30, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, -80.0, 0.01);

    // 4. 晚高峰尖峰放电测试 (19:00, 尖峰放电 +120kW, SOC=70%)
    rc = iems_coordinator_evaluate_cycle(&coord, 350.0, 0.0, 450.0, 0.0, 70.0, 19, 0, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, 120.0, 0.01);

    // 5. 电池高 SOC 满电保护 (02:00 谷段充电, 但 SOC=98% -> 禁止继续充电)
    rc = iems_coordinator_evaluate_cycle(&coord, 100.0, 0.0, 100.0, 0.0, 98.0, 2, 0, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, 0.0, 0.01);

    // 6. 电池低 SOC 亏电保护 (19:00 尖峰放电, 但 SOC=8% -> 禁止继续放电)
    rc = iems_coordinator_evaluate_cycle(&coord, 400.0, 0.0, 400.0, 0.0, 8.0, 19, 0, &planner);
    assert_int_equal(rc, 0);
    assert_float_equal(coord.target_pcs_power_kw, 0.0, 0.01);
}

static void test_dynamic_demand_peak_shaving(void **state) {
    (void)state;

    iems_microgrid_coordinator_t coord;
    // 变压器 630kVA, 需量限制 400kW
    iems_coordinator_init(&coord, 630.0, 400.0);

    iems_tou_planner_t planner;
    iems_coordinator_setup_default_tou(&planner);

    // 厂区负荷突增至 550kW (超出 400kW 需量红线 150kW)
    // 挂钟时间 15:00 (平时排程功率为 0kW)
    int rc = iems_coordinator_evaluate_cycle(&coord, 550.0, 0.0, 550.0, 0.0, 75.0, 15, 0, &planner);
    assert_int_equal(rc, 0);
    assert_true(coord.is_demand_shaving_active);
    // 储能强制放电削峰: target >= 150kW
    assert_true(coord.target_pcs_power_kw >= 150.0);
    assert_true(coord.demand_15min_avg_kw > 0.0);
}

static void test_fast_anti_reverse_power_flow_pid(void **state) {
    (void)state;

    iems_microgrid_coordinator_t coord;
    iems_coordinator_init(&coord, 630.0, 500.0);

    // 场景 A: 光伏大发 (PV=300kW, Load=50kW), 关口功率倒送 (p_grid = -40kW < 5kW 死区)
    // 电池可充电 (SOC=65%) -> 触发防逆流 PID 快速加大充电
    int rc = iems_coordinator_evaluate_cycle(&coord, -40.0, 300.0, 50.0, 0.0, 65.0, 12, 0, NULL);
    assert_int_equal(rc, 0);
    assert_true(coord.is_anti_reverse_active);
    // 充电功率加大 (小于 -45kW)
    assert_true(coord.target_pcs_power_kw <= -45.0);

    // 场景 B: 连续调节 100ms 周期验证 PID 积分与微分效应
    for (int i = 0; i < 5; i++) {
        rc = iems_coordinator_evaluate_cycle(&coord, -30.0, 300.0, 50.0, 0.0, 65.0, 12, 0, NULL);
        assert_int_equal(rc, 0);
        assert_true(coord.is_anti_reverse_active);
    }
    assert_true(coord.pid_integral > 0.0);

    // 场景 C: 电池满电 (SOC=98%), 关口继续倒送 (p_grid = -50kW)
    // 储能无法继续吸收 -> 光伏逆变器快速限额降出力
    iems_coordinator_reset_pid(&coord);
    rc = iems_coordinator_evaluate_cycle(&coord, -50.0, 300.0, 50.0, 0.0, 98.0, 12, 0, NULL);
    assert_int_equal(rc, 0);
    assert_true(coord.is_anti_reverse_active);
    assert_true(coord.target_pv_limit_kw < 300.0);
}

static void test_multi_cycle_closed_loop_simulation(void **state) {
    (void)state;

    iems_microgrid_coordinator_t coord;
    iems_coordinator_init(&coord, 630.0, 500.0);

    iems_tou_planner_t planner;
    iems_coordinator_setup_default_tou(&planner);

    // 模拟 24 小时完整昼夜周期 (每小时采样 10 个 100ms 周期)
    double soc = 50.0;
    for (uint8_t h = 0; h < 24; h++) {
        double pv = (h >= 6 && h <= 18) ? (200.0 * sin((double)(h - 6) * 3.1415926 / 12.0)) : 0.0;
        double load = 100.0 + 150.0 * (double)(h % 6);
        double grid = load - pv - coord.target_pcs_power_kw;

        for (uint8_t step = 0; step < 10; step++) {
            int rc = iems_coordinator_evaluate_cycle(&coord, grid, pv, load, 0.0, soc, h, step * 6,
                                                     &planner);
            assert_int_equal(rc, 0);
        }

        // 动态积分 SOC 变化
        soc -= (coord.target_pcs_power_kw * 0.005);
        if (soc > 98.0) soc = 98.0;
        if (soc < 5.0) soc = 5.0;
    }

    assert_true(coord.demand_window_samples > 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tou_schedule_arbitrage),
        cmocka_unit_test(test_dynamic_demand_peak_shaving),
        cmocka_unit_test(test_fast_anti_reverse_power_flow_pid),
        cmocka_unit_test(test_multi_cycle_closed_loop_simulation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
