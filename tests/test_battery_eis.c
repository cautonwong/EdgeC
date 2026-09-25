#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "battery_eis.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 1. 测试初始化与配置 */
static void test_eis_init_and_config(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);

    const battery_eis_config_t *cfg_ptr = battery_eis_get_config(eis);
    assert_non_null(cfg_ptr);
    assert_int_equal(cfg_ptr->prbs_order, BATTERY_EIS_PRBS_9);
    assert_int_equal(battery_eis_get_prbs_len(eis), 511);
    assert_int_equal(cfg_ptr->num_freq_bins, 16);
    assert_false(battery_eis_is_ready(eis));

    /* 自定义配置 (PRBS-7) */
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_7,
                                .pert_amplitude_a = 2.0,
                                .sample_rate_hz = 500.0,
                                .num_freq_bins = 24,
                                .f_min_hz = 0.05,
                                .f_max_hz = 200.0,
                                .nominal_r0_mohm = 0.60,
                                .nominal_rct_mohm = 1.00};
    int rc = battery_eis_init(eis, &cfg);
    assert_int_equal(rc, 0);
    assert_int_equal(battery_eis_get_prbs_len(eis), 127);
    assert_int_equal(cfg_ptr->num_freq_bins, 24);
    const battery_eis_freq_bin_t *bins = battery_eis_get_bins(eis);
    assert_non_null(bins);
    assert_double_equal(bins[0].freq_hz, 0.05, 0.001);
    assert_true(bins[23].freq_hz >= 195.0);
}

/* 2. 测试 PRBS 扰动发生器的周期与零均值特性 */
static void test_prbs_generator_properties(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_7,
                                .pert_amplitude_a = 1.5,
                                .sample_rate_hz = 1000.0,
                                .num_freq_bins = 8,
                                .f_min_hz = 0.1,
                                .f_max_hz = 100.0,
                                .nominal_r0_mohm = 0.5,
                                .nominal_rct_mohm = 0.8};

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, &cfg);
    assert_non_null(eis);
    double sum = 0.0;
    for (uint32_t i = 0; i < 127; i++) {
        double pert = battery_eis_next_prbs_excitation(eis);
        assert_true(fabs(fabs(pert) - 1.5) < 1e-6);
        sum += pert;
    }

    /* PRBS 7 阶序列有 64 个 1 和 63 个 -1, 均值偏置仅为 1 个步长 (1.5 / 127 ≈ 0.011) */
    double mean = sum / 127.0;
    assert_true(fabs(mean) < 0.02);
}

/* 3. 测试纯电阻阻抗解算 (验证 Z_re = R, Z_im ≈ 0, Phase ≈ 0) */
static void test_pure_resistor_impedance(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_9,
                                .pert_amplitude_a = 1.0,
                                .sample_rate_hz = 1000.0,
                                .num_freq_bins = 12,
                                .f_min_hz = 1.0,
                                .f_max_hz = 200.0,
                                .nominal_r0_mohm = 2.0,
                                .nominal_rct_mohm = 0.0};

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, &cfg);
    assert_non_null(eis);

    double true_r_ohm = 0.0025; /* 2.5 mΩ */
    double v_dc = 3.300;        /* 3.3V DC 基准端压 */

    /* 注入 511 个周期的 PRBS 激励与物理响应 */
    for (uint32_t n = 0; n < 511; n++) {
        double i_pert = battery_eis_next_prbs_excitation(eis);
        double i_total = 0.0 + i_pert;
        double v_meas = v_dc + i_total * true_r_ohm;
        battery_eis_push_sample(eis, i_total, v_meas);
    }

    assert_true(battery_eis_is_ready(eis));
    int rc = battery_eis_solve_spectrum(eis);
    assert_int_equal(rc, 0);

    const battery_eis_freq_bin_t *bins = battery_eis_get_bins(eis);
    assert_non_null(bins);
    /* 校验各频点实部均接近 2.5 mΩ，虚部接近 0 */
    for (uint32_t k = 0; k < cfg.num_freq_bins; k++) {
        assert_true(fabs(bins[k].z_real_mohm - 2.5) < 0.3);
        assert_true(fabs(bins[k].z_imag_mohm) < 0.4);
    }

    rc = battery_eis_fit_nyquist(eis, 25.0, 0.0);
    assert_int_equal(rc, 0);

    const battery_eis_randles_params_t *params = battery_eis_get_params(eis);
    assert_non_null(params);
    assert_true(params->valid);
    assert_true(fabs(params->r0_mohm - 2.5) < 0.4);
}

/* 4. 测试 Randles 等效电路模型 Nyquist 圆弧提取与 R0/Rct 辨识 */
static void test_randles_circuit_nyquist_fitting(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_9,
                                .pert_amplitude_a = 2.0,
                                .sample_rate_hz = 1000.0,
                                .num_freq_bins = 16,
                                .f_min_hz = 0.5,
                                .f_max_hz = 400.0,
                                .nominal_r0_mohm = 0.55,
                                .nominal_rct_mohm = 1.10};

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, &cfg);
    assert_non_null(eis);

    /* 物理真实参数: R0 = 0.55 mΩ, Rct = 1.10 mΩ, Cdl = 10.0 F, tau = 11 ms, fc = 14.5 Hz */
    double r0 = 0.00055;
    double rct = 0.00110;
    double cdl = 10.0;
    double dt = 1.0 / 1000.0;
    double v_rc = 0.0;
    double v_ocv = 3.280;

    /* 运行 511 步 PRBS 动态仿真 */
    for (uint32_t n = 0; n < 511; n++) {
        double i_pert = battery_eis_next_prbs_excitation(eis);
        /* RC 环节微分方程: dV_rc/dt = (I - V_rc/Rct) / Cdl */
        double dv_rc = (i_pert - (v_rc / rct)) / cdl * dt;
        v_rc += dv_rc;

        double v_cell = v_ocv + i_pert * r0 + v_rc;
        battery_eis_push_sample(eis, i_pert, v_cell);
    }

    assert_true(battery_eis_is_ready(eis));
    int rc = battery_eis_solve_spectrum(eis);
    assert_int_equal(rc, 0);

    rc = battery_eis_fit_nyquist(eis, 25.0, 0.0);
    assert_int_equal(rc, 0);

    const battery_eis_randles_params_t *params = battery_eis_get_params(eis);
    assert_non_null(params);
    assert_true(params->valid);
    assert_true(params->r0_mohm >= 0.30 && params->r0_mohm <= 0.90);
    assert_true(params->r_ct_mohm >= 0.50 && params->r_ct_mohm <= 2.00);
    assert_true(params->fit_r2 >= 0.70);
    assert_true(params->soh_eis_pct >= 85.0 && params->soh_eis_pct <= 100.0);
}

/* 5. 测试低温大电流充电下的析锂风险预警 */
static void test_cold_charging_lithium_plating_risk(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);

    const battery_eis_config_t *cfg = battery_eis_get_config(eis);
    assert_non_null(cfg);

    /* 构造模拟阻抗谱数据 */
    battery_eis_freq_bin_t test_bins[BATTERY_EIS_MAX_FREQ_BINS];
    for (uint32_t k = 0; k < cfg->num_freq_bins; k++) {
        test_bins[k].z_real_mohm = 0.60 + 1.20 / (1.0 + (k * 0.2) * (k * 0.2));
        test_bins[k].z_imag_mohm = -(1.20 * 0.5) / (1.0 + (k * 0.1) * (k * 0.1));
    }
    battery_eis_set_test_spectrum(eis, test_bins, cfg->num_freq_bins, 511);

    /* 场景 A: 25°C 常温放电 (+50A) */
    battery_eis_fit_nyquist(eis, 25.0, 50.0);
    const battery_eis_randles_params_t *p_normal = battery_eis_get_params(eis);
    assert_non_null(p_normal);
    assert_true(p_normal->li_plating_risk < 0.10);

    /* 场景 B: -10°C 严寒极速快充 (-140A) */
    battery_eis_fit_nyquist(eis, -10.0, -140.0);
    const battery_eis_randles_params_t *p_cold_chg = battery_eis_get_params(eis);
    assert_non_null(p_cold_chg);
    assert_true(p_cold_chg->li_plating_risk > 0.50); /* 触发析锂极化高危警报 */
}

/* 6. 测试电芯老化内阻倍增与 SOH 衰退评估 */
static void test_soh_degradation_tracking(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);

    const battery_eis_config_t *cfg = battery_eis_get_config(eis);
    assert_non_null(cfg);

    /* 场景 A: 健康新电芯 (R0 = 0.50 mΩ, Rct = 0.80 mΩ) */
    battery_eis_freq_bin_t bins_fresh[BATTERY_EIS_MAX_FREQ_BINS];
    for (uint32_t k = 0; k < cfg->num_freq_bins; k++) {
        bins_fresh[k].z_real_mohm = 0.50 + 0.80 / (1.0 + (k * 0.2) * (k * 0.2));
        bins_fresh[k].z_imag_mohm = -(0.80 * 0.5) / (1.0 + (k * 0.1) * (k * 0.1));
    }
    battery_eis_set_test_spectrum(eis, bins_fresh, cfg->num_freq_bins, 511);
    battery_eis_fit_nyquist(eis, 25.0, 0.0);
    const battery_eis_randles_params_t *p_fresh = battery_eis_get_params(eis);
    assert_non_null(p_fresh);
    assert_true(p_fresh->soh_eis_pct >= 90.0);

    /* 场景 B: 深度老化电芯 (R0 增长 80% 至 0.90 mΩ, Rct 增长 125% 至 1.80 mΩ) */
    battery_eis_freq_bin_t bins_aged[BATTERY_EIS_MAX_FREQ_BINS];
    for (uint32_t k = 0; k < cfg->num_freq_bins; k++) {
        bins_aged[k].z_real_mohm = 0.90 + 1.80 / (1.0 + (k * 0.2) * (k * 0.2));
        bins_aged[k].z_imag_mohm = -(1.80 * 0.5) / (1.0 + (k * 0.1) * (k * 0.1));
    }
    battery_eis_set_test_spectrum(eis, bins_aged, cfg->num_freq_bins, 511);
    battery_eis_fit_nyquist(eis, 25.0, 0.0);
    const battery_eis_randles_params_t *p_aged = battery_eis_get_params(eis);
    assert_non_null(p_aged);
    assert_true(p_aged->soh_eis_pct <= 75.0);
}

/* 7. 测试重置与流式一步接口 Step */
static void test_reset_and_streaming_step(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_7,
                                .pert_amplitude_a = 1.0,
                                .sample_rate_hz = 1000.0,
                                .num_freq_bins = 8,
                                .f_min_hz = 1.0,
                                .f_max_hz = 100.0,
                                .nominal_r0_mohm = 0.5,
                                .nominal_rct_mohm = 0.8};

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, &cfg);
    assert_non_null(eis);

    bool ready = false;
    for (uint32_t i = 0; i < 126; i++) {
        int rc = battery_eis_step(eis, 1.0, 3.301, 25.0, &ready);
        assert_int_equal(rc, 0);
        assert_false(ready);
    }

    /* 第 127 步触发周期闭环 */
    int rc = battery_eis_step(eis, 1.0, 3.301, 25.0, &ready);
    assert_int_equal(rc, 0);
    assert_true(ready);
    assert_non_null(battery_eis_get_params(eis));

    /* 重置后应清零 */
    battery_eis_reset(eis);
    assert_false(battery_eis_is_ready(eis));
    assert_null(battery_eis_get_params(eis));
}

/* 8. 测试 100,000 步超长时采样流数值稳定性 (消除大 t 浮点截断与舍入灾难 AEP-734) */
static void test_long_running_stream_stability_100k(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_config_t cfg = {.prbs_order = BATTERY_EIS_PRBS_7,
                                .pert_amplitude_a = 1.0,
                                .sample_rate_hz = 1000.0,
                                .num_freq_bins = 8,
                                .f_min_hz = 1.0,
                                .f_max_hz = 100.0,
                                .nominal_r0_mohm = 0.5,
                                .nominal_rct_mohm = 0.8};

    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, &cfg);
    assert_non_null(eis);

    double true_r0 = 0.0005;
    double v_dc = 3.300;

    /* 连续推入 100,000 个采样点 (相当于 100 秒持续高频激励) */
    for (uint32_t i = 0; i < 100000; i++) {
        double i_pert = battery_eis_next_prbs_excitation(eis);
        double v_cell = v_dc + i_pert * true_r0;
        battery_eis_push_sample(eis, i_pert, v_cell);
    }

    assert_true(battery_eis_is_ready(eis));
    int rc = battery_eis_solve_spectrum(eis);
    assert_int_equal(rc, 0);

    const battery_eis_freq_bin_t *bins = battery_eis_get_bins(eis);
    assert_non_null(bins);

    /* 校验长时间累加后阻抗谱未发生 NaN 或数值崩溃 */
    for (uint32_t k = 0; k < cfg.num_freq_bins; k++) {
        assert_false(isnan(bins[k].z_real_mohm));
        assert_false(isnan(bins[k].z_imag_mohm));
        assert_false(isinf(bins[k].z_real_mohm));
        assert_false(isinf(bins[k].z_imag_mohm));
        /* 实部应稳定维持在 0.5 mΩ 附近 */
        assert_true(fabs(bins[k].z_real_mohm - 0.5) < 0.2);
    }

    rc = battery_eis_fit_nyquist(eis, 25.0, 0.0);
    assert_int_equal(rc, 0);
    const battery_eis_randles_params_t *params = battery_eis_get_params(eis);
    assert_non_null(params);
    assert_true(params->valid);
    assert_true(fabs(params->r0_mohm - 0.5) < 0.2);
}

/* 9. 测试极端非法温度（低于绝对零度 -273.15°C）防除零拦截 */
static void test_sub_absolute_zero_temperature_defense(void **state) {
    (void)state;
    battery_eis_storage_t store;
    battery_eis_analyzer_t *eis = battery_eis_create_in(&store, NULL);
    assert_non_null(eis);

    battery_eis_set_test_spectrum(eis, NULL, 0, 511);

    /* 传入 -275°C 非法物理超低温 */
    int rc = battery_eis_fit_nyquist(eis, -275.0, 0.0);
    assert_int_equal(rc, -3); /* 必须安全拦截返回 -3 */
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_eis_init_and_config),
        cmocka_unit_test(test_prbs_generator_properties),
        cmocka_unit_test(test_pure_resistor_impedance),
        cmocka_unit_test(test_randles_circuit_nyquist_fitting),
        cmocka_unit_test(test_cold_charging_lithium_plating_risk),
        cmocka_unit_test(test_soh_degradation_tracking),
        cmocka_unit_test(test_reset_and_streaming_step),
        cmocka_unit_test(test_long_running_stream_stability_100k),
        cmocka_unit_test(test_sub_absolute_zero_temperature_defense),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
