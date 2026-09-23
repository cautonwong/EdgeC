/**
 * @file test_dcu_power_quality.c
 * @brief Empirical Verification for DCU Power Quality & Phase Switching Engine (AEP-441)
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

#include "dcu_power_quality.h"

static void test_three_phase_unbalance_calculation(void **state) {
    (void)state;

    // 1. 完全平衡 (0.0%)
    double unb = dcu_calc_three_phase_unbalance(100.0, 100.0, 100.0);
    assert_float_equal(unb, 0.0, 0.001);

    // 2. 中度不平衡 (Ia=120, Ib=100, Ic=80 -> Avg=100, MaxDiff=20 -> 20.0%)
    unb = dcu_calc_three_phase_unbalance(120.0, 100.0, 80.0);
    assert_float_equal(unb, 20.0, 0.001);

    // 3. 严重不平衡 (Ia=180, Ib=90, Ic=30 -> Avg=100, MaxDiff=80 -> 80.0%)
    unb = dcu_calc_three_phase_unbalance(180.0, 90.0, 30.0);
    assert_float_equal(unb, 80.0, 0.001);

    // 4. 零电流安全保护
    unb = dcu_calc_three_phase_unbalance(0.0, 0.0, 0.0);
    assert_float_equal(unb, 0.0, 0.001);
}

static void test_phase_switching_decision_solver(void **state) {
    (void)state;

    dcu_power_quality_engine_t engine;
    dcu_power_quality_init(&engine, 220.0, 15.0);  // 15% 门限

    // 设定台区初始三相电流: Ia=180A, Ib=80A, Ic=40A (Avg=100A, Unbalance=80%)
    engine.ia = 180.0;
    engine.ib = 80.0;
    engine.ic = 40.0;

    // A 相上的可换相单相表箱支路
    dcu_switchable_branch_t branches[3] = {
        {.meter_id = 101, .current_phase = PQ_PHASE_A, .current_amps = 15.0},
        {.meter_id = 102, .current_phase = PQ_PHASE_A, .current_amps = 60.0},
        {.meter_id = 103, .current_phase = PQ_PHASE_A, .current_amps = 30.0},
    };

    dcu_phase_switch_plan_t plan;
    int rc = dcu_solve_phase_switch_plan(&engine, branches, 3, &plan);
    assert_int_equal(rc, 0);

    // 验证决策结果
    assert_true(plan.is_action_recommended);
    assert_int_equal(plan.action_count, 1);
    assert_int_equal(plan.actions[0].meter_id, 102);  // 迁移 60A 表箱收益最大
    assert_int_equal(plan.actions[0].from_phase, PQ_PHASE_A);
    assert_int_equal(plan.actions[0].to_phase, PQ_PHASE_C);  // 迁往负荷最小的 C 相
    assert_float_equal(plan.actions[0].migrated_current_amps, 60.0, 0.01);

    // 迁移后: Ia'=120A, Ib'=80A, Ic'=100A -> Unbalance'=20%
    assert_float_equal(plan.initial_unbalance_pct, 80.0, 0.01);
    assert_float_equal(plan.projected_unbalance_pct, 20.0, 0.01);
}

static void test_2_to_31_harmonics_and_thd_computation(void **state) {
    (void)state;

    double fundamental = 220.0;
    double harmonics[DCU_MAX_HARMONICS] = {0};

    // 注入典型谐波分量 (3次、5次、7次奇次谐波 + 2次偶次微量)
    harmonics[2] = 2.2;   // 1.0%
    harmonics[3] = 11.0;  // 5.0%
    harmonics[5] = 8.8;   // 4.0%
    harmonics[7] = 6.6;   // 3.0%

    dcu_harmonic_analysis_t analysis;
    int rc = dcu_analyze_harmonics_thd(fundamental, harmonics, &analysis);
    assert_int_equal(rc, 0);

    // 验证各次含有率
    assert_float_equal(analysis.harmonic_pct[2], 1.0, 0.01);
    assert_float_equal(analysis.harmonic_pct[3], 5.0, 0.01);
    assert_float_equal(analysis.harmonic_pct[5], 4.0, 0.01);
    assert_float_equal(analysis.harmonic_pct[7], 3.0, 0.01);

    // 验证总 THD = sqrt(2.2^2 + 11^2 + 8.8^2 + 6.6^2) / 220 * 100% = 15.72 / 220 = 7.147%
    assert_true(analysis.thd_total_pct > 7.0 && analysis.thd_total_pct < 7.3);
    assert_true(analysis.thd_odd_pct > 6.9);
    assert_float_equal(analysis.thd_even_pct, 1.0, 0.01);
}

static void test_voltage_transient_sag_swell_interrupt_detection(void **state) {
    (void)state;

    dcu_power_quality_engine_t engine;
    dcu_power_quality_init(&engine, 220.0, 15.0);

    dcu_transient_event_t event;

    // 1. 正常稳态电压 220V
    int rc = dcu_detect_voltage_transient(&engine, 220.0, 1000, &event);
    assert_int_equal(rc, 0);
    assert_int_equal(event.event_type, PQ_EVENT_NONE);

    // 2. 电压暂降 (跌落至 150V < 198V)
    rc = dcu_detect_voltage_transient(&engine, 150.0, 1020, &event);
    assert_int_equal(rc, 0);
    assert_int_equal(event.event_type, PQ_EVENT_VOLTAGE_SAG);
    assert_true(event.is_active);
    assert_float_equal(event.extreme_voltage, 150.0, 0.01);

    // 3. 暂降持续中 (进一步跌至 140V, t=1120ms)
    rc = dcu_detect_voltage_transient(&engine, 140.0, 1120, &event);
    assert_int_equal(rc, 0);
    assert_int_equal(event.event_type, PQ_EVENT_VOLTAGE_SAG);
    assert_true(event.is_active);
    assert_int_equal(event.duration_ms, 100);
    assert_float_equal(event.extreme_voltage, 140.0, 0.01);

    // 4. 电压恢复 (恢复至 220V, t=1220ms)
    rc = dcu_detect_voltage_transient(&engine, 220.0, 1220, &event);
    assert_int_equal(rc, 0);
    assert_false(event.is_active);
    assert_int_equal(event.duration_ms, 200);

    // 5. 电压暂升 (骤升至 260V > 242V)
    rc = dcu_detect_voltage_transient(&engine, 260.0, 2000, &event);
    assert_int_equal(rc, 0);
    assert_int_equal(event.event_type, PQ_EVENT_VOLTAGE_SWELL);
    assert_true(event.is_active);
    assert_float_equal(event.extreme_voltage, 260.0, 0.01);

    // 6. 短时中断 (< 22V)
    dcu_power_quality_init(&engine, 220.0, 15.0);
    rc = dcu_detect_voltage_transient(&engine, 10.0, 3000, &event);
    assert_int_equal(rc, 0);
    assert_int_equal(event.event_type, PQ_EVENT_INTERRUPTION);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_three_phase_unbalance_calculation),
        cmocka_unit_test(test_phase_switching_decision_solver),
        cmocka_unit_test(test_2_to_31_harmonics_and_thd_computation),
        cmocka_unit_test(test_voltage_transient_sag_swell_interrupt_detection),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
