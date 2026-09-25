#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "battery_micro_isc.h"

static void test_init_and_config(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = (pack_micro_isc_detector_t *)store.raw;

    /* 边界测试：空指针与越界数量 */
    assert_int_equal(battery_micro_isc_init(NULL, NULL, 16), -1);
    assert_int_equal(battery_micro_isc_init(det, NULL, 0), -1);
    assert_int_equal(battery_micro_isc_init(det, NULL, BATTERY_MAX_CELLS_PER_PACK + 1), -1);

    /* 默认配置初始化 */
    det = battery_micro_isc_create_in(&store, NULL, 16, NULL);
    assert_non_null(det);
    assert_int_equal(battery_micro_isc_get_cell_count(det), 16);
    const battery_micro_isc_config_t *cfg = battery_micro_isc_get_config(det);
    assert_non_null(cfg);
    assert_float_equal(cfg->nominal_capacity_ah, 280.0f, 0.001f);
    assert_float_equal(cfg->warn_voltage_diff_v, 0.015f, 0.0001f);
    assert_float_equal(cfg->alarm_voltage_diff_v, 0.035f, 0.0001f);
    assert_float_equal(cfg->warn_r_isc_ohm, 1000.0f, 0.01f);
    assert_float_equal(cfg->alarm_r_isc_ohm, 200.0f, 0.01f);

    /* 自定义配置初始化 */
    battery_micro_isc_config_t custom_cfg = {.nominal_capacity_ah = 314.0f,
                                             .sample_interval_s = 2.0f,
                                             .warn_voltage_diff_v = 0.010f,
                                             .alarm_voltage_diff_v = 0.030f,
                                             .warn_r_isc_ohm = 1500.0f,
                                             .alarm_r_isc_ohm = 300.0f,
                                             .critical_r_isc_ohm = 60.0f,
                                             .li_plating_d2v_threshold = 0.00015f,
                                             .rest_current_threshold_a = 0.1f};
    det = battery_micro_isc_create_in(&store, &custom_cfg, 32, NULL);
    assert_non_null(det);
    assert_int_equal(battery_micro_isc_get_cell_count(det), 32);
    cfg = battery_micro_isc_get_config(det);
    assert_non_null(cfg);
    assert_float_equal(cfg->nominal_capacity_ah, 314.0f, 0.001f);
}

static void test_healthy_pack_baseline(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 16, NULL);
    assert_non_null(det);

    /* 模拟 16 个健康电芯静置 3600s */
    double t = 0.0;
    for (int step = 0; step < 60; step++) {
        t += 60.0; /* 步长 60s */
        for (uint16_t i = 0; i < 16; i++) {
            /* 正常平滑指数衰减电压 */
            float v = 3.350f - 0.010f * (1.0f - expf(-(float)t / 600.0f)) + (float)i * 0.0001f;
            float soc = 0.800f - 0.00001f * (float)(t / 3600.0); /* 极低自然自放电 */
            assert_int_equal(battery_micro_isc_feed_cell(det, i, v, 0.0f, 25.0f, soc, t), 0);
        }
        assert_int_equal(battery_micro_isc_process_step(det, t), 0);
    }

    battery_isc_alarm_level_t max_alarm;
    uint16_t worst_cell;
    assert_int_equal(battery_micro_isc_get_highest_alarm(det, &max_alarm, &worst_cell), 0);
    assert_int_equal(max_alarm, BATTERY_ALARM_NORMAL);

    cell_isc_state_t report;
    assert_int_equal(battery_micro_isc_get_cell_report(det, 0, &report), 0);
    assert_int_equal(report.alarm_level, BATTERY_ALARM_NORMAL);
    assert_true(report.r_isc_ohm >= 100000.0f);
    assert_float_equal(report.estimated_days_to_failure, 30.0f, 0.1f);
    assert_false(report.relaxation.li_plating_detected);
}

static void test_lithium_plating_detection(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 8, NULL);
    assert_non_null(det);

    /* Cell 2 发生轻度/中度析锂：在充电结束松弛初期出现剥离相变平台 (d2V/dt2 异常反弯) */
    double t = 0.0;
    for (int step = 0; step < 30; step++) {
        t += 10.0; /* 每步 10s */
        for (uint16_t i = 0; i < 8; i++) {
            float v = 3.400f - 0.015f * (1.0f - expf(-(float)t / 200.0f));
            if (i == 2 && step >= 8 && step <= 16) {
                /* 注入析锂金属溶解剥离电压平台波动 */
                v += 0.008f * sinf((float)(step - 8) * 0.4f);
            }
            assert_int_equal(battery_micro_isc_feed_cell(det, i, v, 0.0f, 25.0f, 0.90f, t), 0);
        }
        assert_int_equal(battery_micro_isc_process_step(det, t), 0);
    }

    cell_isc_state_t report_c2;
    assert_int_equal(battery_micro_isc_get_cell_report(det, 2, &report_c2), 0);
    assert_true(report_c2.relaxation.li_plating_detected);
    const battery_micro_isc_config_t *cfg = battery_micro_isc_get_config(det);
    assert_non_null(cfg);
    assert_true(report_c2.relaxation.d2v_dt2_max_residual >= cfg->li_plating_d2v_threshold);
    assert_true(report_c2.alarm_level >= BATTERY_ALARM_EARLY_WARNING_7D);
    assert_true(report_c2.estimated_days_to_failure <= 7.0f);

    /* 其余健康电芯无析锂 */
    cell_isc_state_t report_c0;
    assert_int_equal(battery_micro_isc_get_cell_report(det, 0, &report_c0), 0);
    assert_false(report_c0.relaxation.li_plating_detected);
}

static void test_micro_isc_resistance_identification(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    battery_micro_isc_config_t cfg = {.nominal_capacity_ah = 280.0f,
                                      .sample_interval_s = 60.0f,
                                      .warn_voltage_diff_v = 0.015f,
                                      .alarm_voltage_diff_v = 0.035f,
                                      .warn_r_isc_ohm = 1000.0f,
                                      .alarm_r_isc_ohm = 200.0f,
                                      .critical_r_isc_ohm = 50.0f,
                                      .li_plating_d2v_threshold = 0.0002f,
                                      .rest_current_threshold_a = 0.2f};
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, &cfg, 16, NULL);
    assert_non_null(det);

    /* 设定真实物理微短路电阻: R_isc = 400 Ω
     * 电芯电压 V = 3.30 V
     * 短路漏电流 I_leak = 3.30 / 400 = 0.00825 A
     * 静置时间 2 小时 = 7200 秒
     * 损失容量 = 0.00825 A * 2 h = 0.0165 Ah
     * 标称容量 280 Ah 下，额外 SOC 降 = 0.0165 / 280 = 5.892857e-5
     */
    const float true_r_isc = 400.0f;
    const float v_cell = 3.30f;
    const float q_nom = 280.0f;
    const float rest_s = 7200.0f;
    const float i_leak = v_cell / true_r_isc;
    const float excess_delta_soc = (i_leak * (rest_s / 3600.0f)) / q_nom;

    double t = 0.0;
    /* 第 0 步: 进入静置初始采样 */
    for (uint16_t i = 0; i < 16; i++) {
        assert_int_equal(battery_micro_isc_feed_cell(det, i, 3.30f, 0.0f, 25.0f, 0.8000f, 0.0), 0);
    }
    assert_int_equal(battery_micro_isc_process_step(det, 0.0), 0);

    /* 推进到 7200s (2小时后) */
    t = 7200.0;
    for (uint16_t i = 0; i < 16; i++) {
        float soc = 0.8000f - 0.0001f; /* 正常基准自放电 */
        float v = 3.300f;
        if (i == 7) {
            /* 7号电芯微短路 */
            soc -= excess_delta_soc;
            v -= 0.006f; /* 微压降 6mV */
        }
        assert_int_equal(battery_micro_isc_feed_cell(det, i, v, 0.0f, 25.0f, soc, t), 0);
    }
    assert_int_equal(battery_micro_isc_process_step(det, t), 0);

    cell_isc_state_t report_c7;
    assert_int_equal(battery_micro_isc_get_cell_report(det, 7, &report_c7), 0);

    /* 辨识出的 R_isc 必须极其精确，误差 < 5% */
    float r_err = fabsf(report_c7.r_isc_ohm - true_r_isc) / true_r_isc;
    assert_true(r_err < 0.05f);

    /* 400Ω 处于 200Ω ~ 1000Ω 之间，必须精准触发 3 天早期预警 */
    assert_int_equal(report_c7.alarm_level, BATTERY_ALARM_EARLY_WARNING_3D);
    assert_true(report_c7.estimated_days_to_failure >= 2.0f &&
                report_c7.estimated_days_to_failure <= 4.0f);

    /* 获取最高告警判定 */
    battery_isc_alarm_level_t max_alarm;
    uint16_t worst_cell;
    assert_int_equal(battery_micro_isc_get_highest_alarm(det, &max_alarm, &worst_cell), 0);
    assert_int_equal(max_alarm, BATTERY_ALARM_EARLY_WARNING_3D);
    assert_int_equal(worst_cell, 7);
}

static void test_severe_isc_1d_and_critical(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 16, NULL);
    assert_non_null(det);

    /* Cell 3: 严重微短路 R_isc = 120Ω (50Ω ~ 200Ω) -> 触发 1 天紧急告警 */
    /* Cell 5: 极度危险硬短路 R_isc = 30Ω (< 50Ω) 且压差 40mV (> 35mV) -> 触发立即危险告警 */
    double t = 3600.0;

    for (uint16_t i = 0; i < 16; i++) {
        battery_micro_isc_feed_cell(det, i, 3.30f, 0.0f, 25.0f, 0.80f, 0.0);
    }
    battery_micro_isc_process_step(det, 0.0);

    /* 3600s 数据 */
    for (uint16_t i = 0; i < 16; i++) {
        float soc = 0.80f - 0.0001f;
        float v = 3.30f;
        if (i == 3) {
            /* 120Ω -> I_leak = 3.3/120 = 0.0275 A -> Ah_loss = 0.0275 -> delta_soc = 0.0275/280 */
            soc -= (0.0275f / 280.0f);
            v -= 0.018f; /* 压差 18mV */
        } else if (i == 5) {
            /* 30Ω -> I_leak = 3.3/30 = 0.11 A -> Ah_loss = 0.11 -> delta_soc = 0.11/280 */
            soc -= (0.11f / 280.0f);
            v -= 0.040f; /* 压差 40mV > 35mV */
        }
        battery_micro_isc_feed_cell(det, i, v, 0.0f, 25.0f, soc, t);
    }
    battery_micro_isc_process_step(det, t);

    cell_isc_state_t report_c3, report_c5;
    assert_int_equal(battery_micro_isc_get_cell_report(det, 3, &report_c3), 0);
    assert_int_equal(battery_micro_isc_get_cell_report(det, 5, &report_c5), 0);

    assert_int_equal(report_c3.alarm_level, BATTERY_ALARM_URGENT_1D);
    assert_true(report_c3.estimated_days_to_failure <= 2.0f);

    assert_int_equal(report_c5.alarm_level, BATTERY_ALARM_CRITICAL_IMMEDIATE);
    assert_true(report_c5.estimated_days_to_failure <= 0.5f);

    battery_isc_alarm_level_t max_alarm;
    uint16_t worst_cell;
    assert_int_equal(battery_micro_isc_get_highest_alarm(det, &max_alarm, &worst_cell), 0);
    assert_int_equal(max_alarm, BATTERY_ALARM_CRITICAL_IMMEDIATE);
    assert_int_equal(worst_cell, 5);
}

static void test_multi_cell_pack_robustness_64_cells(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 64, NULL);
    assert_non_null(det);
    assert_int_equal(battery_micro_isc_get_cell_count(det), 64);

    /* 喂入 64 电芯轻微白噪声数据 */
    for (uint16_t i = 0; i < 64; i++) {
        float noise = ((float)(i % 7) - 3.0f) * 0.0005f;
        assert_int_equal(battery_micro_isc_feed_cell(det, i, 3.32f + noise, 0.0f,
                                                     24.0f + (float)(i % 5) * 0.5f, 0.75f, 100.0),
                         0);
    }
    assert_int_equal(battery_micro_isc_process_step(det, 100.0), 0);

    battery_isc_alarm_level_t max_alarm;
    uint16_t worst_cell;
    assert_int_equal(battery_micro_isc_get_highest_alarm(det, &max_alarm, &worst_cell), 0);
    assert_int_equal(max_alarm, BATTERY_ALARM_NORMAL);

    /* 重置并重新验证 */
    battery_micro_isc_reset(det);
    assert_int_equal(battery_micro_isc_get_cell_count(det), 64);
    assert_int_equal(battery_micro_isc_get_total_samples(det), 0);
}

static void test_nan_cell_and_qsort_median_defense(void **state) {
    (void)state;
    pack_micro_isc_detector_storage_t store;
    pack_micro_isc_detector_t *det = battery_micro_isc_create_in(&store, NULL, 16, NULL);
    assert_non_null(det);

    /* 1. 注入包含 NaN 的电压与 SOC，测试 qsort compare_floats 严格弱序与中位数计算 */
    for (uint16_t i = 0; i < 16; i++) {
        float v = (i == 4) ? NAN : (3.30f + 0.001f * (float)i);
        float s = (i == 9) ? NAN : 0.80f;
        assert_int_equal(battery_micro_isc_feed_cell(det, i, v, 0.0f, 25.0f, s, 100.0), 0);
    }
    assert_int_equal(battery_micro_isc_process_step(det, 100.0), 0);

    assert_true(isfinite(battery_micro_isc_get_pack_avg_voltage(det)));
    assert_true(isfinite(battery_micro_isc_get_pack_median_voltage(det)));
    assert_true(battery_micro_isc_get_pack_median_voltage(det) > 3.20f && battery_micro_isc_get_pack_median_voltage(det) < 3.40f);

    /* 2. 测试 battery_micro_isc_identify_r_isc 输入 NaN rest_time_s 防除零 */
    cell_isc_state_t test_cell;
    memset(&test_cell, 0, sizeof(test_cell));
    test_cell.r_isc_ohm = 1000000.0f;
    float out_r = 0.0f;
    float out_rate = 0.0f;
    assert_int_equal(
        battery_micro_isc_identify_r_isc(&test_cell, 3.30f, 0.001f, 280.0f, NAN, &out_r, &out_rate),
        0);
    assert_true(isfinite(out_r));
    assert_true(isfinite(out_rate));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_and_config),
        cmocka_unit_test(test_healthy_pack_baseline),
        cmocka_unit_test(test_lithium_plating_detection),
        cmocka_unit_test(test_micro_isc_resistance_identification),
        cmocka_unit_test(test_severe_isc_1d_and_critical),
        cmocka_unit_test(test_multi_cell_pack_robustness_64_cells),
        cmocka_unit_test(test_nan_cell_and_qsort_median_defense),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
