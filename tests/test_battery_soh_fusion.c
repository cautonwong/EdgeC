#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <cmocka.h>

#include "battery_soh_fusion.h"

/* 1. 测试初始化与配置校验 */
static void test_soh_init_and_config(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;

    /* 默认初始化 */
    int rc = battery_soh_init(&est, NULL);
    assert_int_equal(rc, 0);
    assert_double_equal(est.config.nominal_capacity_ah, 280.0, 0.01);
    assert_double_equal(est.config.fresh_r0_mohm, 0.50, 0.01);
    assert_double_equal(est.config.eol_soh_ratio, 0.80, 0.01);
    assert_double_equal(est.config.min_coulomb_delta_soc, 0.25, 0.01);

    /* 自定义配置 */
    battery_soh_config_t cfg = {
        .nominal_capacity_ah = 314.0,
        .fresh_r0_mohm = 0.40,
        .fresh_rct_mohm = 0.70,
        .eol_soh_ratio = 0.75,
        .min_coulomb_delta_soc = 0.30
    };
    rc = battery_soh_init(&est, &cfg);
    assert_int_equal(rc, 0);
    assert_double_equal(est.config.nominal_capacity_ah, 314.0, 0.01);
    assert_double_equal(est.config.eol_soh_ratio, 0.75, 0.01);
}

/* 2. 测试出厂新电芯基准与优秀评级 */
static void test_fresh_cell_baseline_grade(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    /* 喂入初始 10 秒运行数据 (常温 25°C, SOC 0.50) */
    for (int i = 0; i < 10; i++) {
        double t = (double)i * 1.0;
        battery_soh_feed_sample(&est, 0.0, 3.295, 25.0, 0.50, t);
    }

    int rc = battery_soh_process_fusion(&est, 10.0);
    assert_int_equal(rc, 0);

    const battery_soh_result_t *res = battery_soh_get_result(&est);
    assert_non_null(res);
    assert_true(res->valid);
    assert_true(res->soh_capacity >= 0.98);
    assert_int_equal(res->health_grade, BATTERY_SOH_GRADE_EXCELLENT);
    assert_true(res->confidence_score >= 0.80);
}

/* 3. 测试区间库仑积分动态容量辨识 */
static void test_coulomb_counting_capacity_identification(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    /* 模拟从 SOC 0.20 恒流充电至 SOC 0.80 (ΔSOC = 0.60) */
    /* 电流 -140A (0.5C), 持续时间 4320 秒 (1.2 小时) */
    /* 吞吐电量 = 140 * 1.2 = 168 Ah -> 辨识容量 = 168 / 0.60 = 280 Ah */
    double dt = 1.0;
    double current = -140.0;
    double total_sec = 4320.0;
    uint32_t steps = (uint32_t)(total_sec / dt);

    for (uint32_t i = 0; i <= steps; i++) {
        double t = (double)i * dt;
        double progress = (double)i / (double)steps;
        double soc = 0.20 + 0.60 * progress;
        double v = 3.20 + 0.20 * progress;
        battery_soh_feed_sample(&est, current, v, 28.0, soc, t);
    }

    /* 校验库仑积分辨识有效 */
    assert_true(est.coulomb.last_coulomb_valid);
    assert_true(fabs(est.coulomb.last_identified_cap_ah - 280.0) < 2.0);
    assert_true(fabs(est.coulomb.last_coulomb_soh - 1.0) < 0.02);

    int rc = battery_soh_process_fusion(&est, total_sec);
    assert_int_equal(rc, 0);

    const battery_soh_result_t *res = battery_soh_get_result(&est);
    assert_non_null(res);
    assert_true(res->soh_coulomb >= 0.98);
    assert_true(res->soh_capacity >= 0.96);
    assert_true(res->confidence_score >= 0.90);
}

/* 4. 测试长期循环与日历老化衰减及 RUL 寿命预测 */
static void test_long_term_cycle_and_calendar_aging(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    /* 模拟已服役 400 天，累计 1200 次等效循环 (总吞吐 672,000 Ah) */
    est.initial_timestamp_s = 1000.0;
    est.last_timestamp_s = 1000.0 + 400.0 * 86400.0;
    est.total_ah_charge = 336000.0;
    est.total_ah_discharge = 336000.0;
    est.total_energy_kwh = 2100000.0;
    est.avg_operating_temp_c = 32.0;
    est.total_samples_fed = 100000;

    /* 喂入老化的内阻数据 (R0 从 0.50 增长到 0.75 mΩ, 增长 50%) */
    battery_soh_feed_impedance(&est, 0.75, 1.40, 32.0);

    int rc = battery_soh_process_fusion(&est, est.last_timestamp_s);
    assert_int_equal(rc, 0);

    const battery_soh_result_t *res = battery_soh_get_result(&est);
    assert_non_null(res);
    assert_true(res->valid);
    /* 1200 循环与 400 天后 SOH 应位于 85% ~ 94% 范围 */
    assert_true(res->soh_capacity >= 0.85 && res->soh_capacity <= 0.94);
    assert_true(res->total_equivalent_cycles >= 1190.0 && res->total_equivalent_cycles <= 1210.0);
    assert_true(res->rul_remaining_cycles > 500.0); /* 剩余循环数显著大于 0 */
    assert_true(res->rul_remaining_days > 150.0);
}

/* 5. 测试达到 80% EOL 退役门限判定 */
static void test_eol_retirement_threshold(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    /* 模拟超期服役 1200 天，累计 3500 次等效循环 */
    est.initial_timestamp_s = 1000.0;
    est.last_timestamp_s = 1000.0 + 1200.0 * 86400.0;
    est.total_ah_charge = 980000.0;
    est.total_ah_discharge = 980000.0;
    est.avg_operating_temp_c = 38.0;
    est.total_samples_fed = 500000;

    /* 喂入重度老化的内阻数据 (R0 = 1.25 mΩ, 增长 150%) */
    battery_soh_feed_impedance(&est, 1.25, 2.50, 38.0);

    int rc = battery_soh_process_fusion(&est, est.last_timestamp_s);
    assert_int_equal(rc, 0);

    const battery_soh_result_t *res = battery_soh_get_result(&est);
    assert_non_null(res);
    /* SOH 应低于或接近 80% EOL */
    assert_true(res->soh_capacity <= 0.82);
    assert_true(res->rul_remaining_cycles < 200.0);
    if (res->soh_capacity < 0.80) {
        assert_int_equal(res->health_grade, BATTERY_SOH_GRADE_EOL);
    }
}

/* 6. 测试 ICA 增量容量相变峰值提取 */
static void test_ica_phase_peak_extraction(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    /* 模拟充电过程中在 3.32V 处出现平台期 (电压上升极其缓慢，对应 dQ/dV 峰值) */
    double ah = 0.0;
    for (int i = 0; i < 40; i++) {
        double v = (i < 20) ? (3.30 + i * 0.001) : (3.32 + (i - 20) * 0.005);
        ah += 2.0;
        battery_soh_feed_sample(&est, -100.0, v, 25.0, 0.40, (double)i * 10.0);
    }

    /* 校验 ICA 队列与采样特征有效 */
    assert_true(est.ica.sample_count >= 16);
    assert_true(est.ica.features_valid);
    assert_true(est.ica.peak1_voltage_v >= 3.28 && est.ica.peak1_voltage_v <= 3.36);
}

/* 7. 测试重置与状态一致性 */
static void test_reset_and_consistency(void **state)
{
    (void)state;
    battery_soh_fusion_estimator_t est;
    battery_soh_init(&est, NULL);

    battery_soh_feed_sample(&est, -50.0, 3.30, 25.0, 0.50, 100.0);
    battery_soh_process_fusion(&est, 100.0);
    assert_true(est.result.valid);

    battery_soh_reset(&est);
    assert_false(est.result.valid);
    assert_double_equal(est.total_ah_charge, 0.0, 0.001);
    assert_null(battery_soh_get_result(&est));
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_soh_init_and_config),
        cmocka_unit_test(test_fresh_cell_baseline_grade),
        cmocka_unit_test(test_coulomb_counting_capacity_identification),
        cmocka_unit_test(test_long_term_cycle_and_calendar_aging),
        cmocka_unit_test(test_eol_retirement_threshold),
        cmocka_unit_test(test_ica_phase_peak_extraction),
        cmocka_unit_test(test_reset_and_consistency),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
