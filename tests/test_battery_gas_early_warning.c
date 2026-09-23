#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "battery_gas_early_warning.h"

/* 1. 测试初始化与配置校验 */
static void test_ew_init_and_config(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;

    /* 默认配置 */
    int rc = battery_gas_ew_init(&ew, NULL);
    assert_int_equal(rc, 0);
    assert_double_equal(ew.config.voc_trace_ppb, 300.0, 0.01);
    assert_double_equal(ew.config.co_vent_ppm, 50.0, 0.01);
    assert_double_equal(ew.config.h2_vent_ppm, 80.0, 0.01);
    assert_double_equal(ew.config.smoke_runaway_thresh, 0.15, 0.01);

    /* 自定义配置 */
    battery_gas_ew_config_t cfg = {.voc_trace_ppb = 500.0,
                                   .h2_trace_ppm = 25.0,
                                   .co_trace_ppm = 15.0,
                                   .co_vent_ppm = 80.0,
                                   .h2_vent_ppm = 120.0,
                                   .pressure_pulse_pa_s = 50.0,
                                   .temp_rate_vent_cs = 0.50,
                                   .smoke_runaway_thresh = 0.20,
                                   .co_runaway_ppm = 300.0,
                                   .temp_runaway_c = 80.0,
                                   .max_valid_temp_c = 160.0,
                                   .min_valid_pressure_pa = 50000.0,
                                   .max_valid_pressure_pa = 160000.0};
    rc = battery_gas_ew_init(&ew, &cfg);
    assert_int_equal(rc, 0);
    assert_double_equal(ew.config.voc_trace_ppb, 500.0, 0.01);
    assert_double_equal(ew.config.co_runaway_ppm, 300.0, 0.01);
}

/* 2. 测试正常环境背景与安全待机 */
static void test_normal_background_safe(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 连续喂入 10 组正常数据 */
    for (int i = 0; i < 10; i++) {
        battery_gas_sensor_sample_t s = {.co_ppm = 1.2 + 0.1 * (i % 3),
                                         .h2_ppm = 2.0 + 0.1 * (i % 2),
                                         .voc_ppb = 45.0 + (i % 5),
                                         .pressure_pa = 101325.0 + (i % 3),
                                         .smoke_density = 0.01,
                                         .max_cell_temp_c = 25.0 + 0.1 * i,
                                         .min_cell_volt_v = 3.300,
                                         .timestamp_s = (double)i * 1.0};
        battery_gas_ew_push_sample(&ew, &s);
    }

    int rc = battery_gas_ew_process_step(&ew, 10.0);
    assert_int_equal(rc, 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_true(rep->valid);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_NORMAL);
    assert_false(rep->trip_contactor_req);
    assert_false(rep->start_exhaust_fan_req);
    assert_false(rep->trigger_extinguisher_req);
    assert_true(rep->estimated_seconds_to_runaway > 10000.0);
}

/* 3. 测试阶段 1: 喷阀前微量气体潜伏预警 (VOC/H2 初渗) */
static void test_stage1_prevent_trace_detection(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 样本 1: 正常基线 */
    battery_gas_sensor_sample_t s1 = {.co_ppm = 1.0,
                                      .h2_ppm = 2.0,
                                      .voc_ppb = 50.0,
                                      .pressure_pa = 101325.0,
                                      .smoke_density = 0.01,
                                      .max_cell_temp_c = 26.0,
                                      .min_cell_volt_v = 3.30,
                                      .timestamp_s = 10.0};
    battery_gas_ew_push_sample(&ew, &s1);

    /* 样本 2: VOC 显著抬升至 450 ppb, H2 抬升至 22 ppm */
    battery_gas_sensor_sample_t s2 = {.co_ppm = 3.5,
                                      .h2_ppm = 22.0,
                                      .voc_ppb = 450.0,
                                      .pressure_pa = 101330.0,
                                      .smoke_density = 0.02,
                                      .max_cell_temp_c = 42.0,
                                      .min_cell_volt_v = 3.28,
                                      .timestamp_s = 15.0};
    battery_gas_ew_push_sample(&ew, &s2);

    int rc = battery_gas_ew_process_step(&ew, 15.0);
    assert_int_equal(rc, 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_STAGE1_PREVENT_TRACE);
    assert_true(rep->prob_prevent_latent >= 0.40);
    assert_true(rep->start_exhaust_fan_req); /* 触发排风防爆稀释 */
    assert_false(rep->trip_contactor_req);
    assert_false(rep->trigger_extinguisher_req);
    assert_true(rep->estimated_seconds_to_runaway >= 600.0);
}

/* 4. 测试阶段 2: 安全阀喷阀确认 (CO/H2 突增 + 气压脉冲) */
static void test_stage2_venting_pulse_confirmation(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 样本 1: 喷阀前夕 */
    battery_gas_sensor_sample_t s1 = {.co_ppm = 5.0,
                                      .h2_ppm = 10.0,
                                      .voc_ppb = 200.0,
                                      .pressure_pa = 101325.0,
                                      .smoke_density = 0.02,
                                      .max_cell_temp_c = 48.0,
                                      .min_cell_volt_v = 3.26,
                                      .timestamp_s = 20.0};
    battery_gas_ew_push_sample(&ew, &s1);

    /* 样本 2: 破裂喷阀瞬间 (气压突增 80 Pa/s, CO 85 ppm, H2 140 ppm, 温升 0.4°C/s) */
    battery_gas_sensor_sample_t s2 = {.co_ppm = 85.0,
                                      .h2_ppm = 140.0,
                                      .voc_ppb = 3500.0,
                                      .pressure_pa = 101405.0,
                                      .smoke_density = 0.05,
                                      .max_cell_temp_c = 48.8,
                                      .min_cell_volt_v = 3.20,
                                      .timestamp_s = 22.0};
    battery_gas_ew_push_sample(&ew, &s2);

    int rc = battery_gas_ew_process_step(&ew, 22.0);
    assert_int_equal(rc, 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_STAGE2_VENT_CONFIRM);
    assert_true(rep->prob_vent_confirmed >= 0.45);
    assert_true(rep->dpressure_dt_pa_s >= 35.0);
    assert_true(rep->trip_contactor_req); /* 确认喷阀立即分断主回路 */
    assert_true(rep->start_exhaust_fan_req);
    assert_false(rep->trigger_extinguisher_req);
    assert_true(rep->estimated_seconds_to_runaway <= 400.0);
}

/* 5. 测试阶段 3: 热失控剧烈起火临界 (烟雾爆发 + 高温失控 + 灭火喷放) */
static void test_stage3_runaway_fire_imminent(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 样本: 烟雾浓度 0.35, CO 420 ppm, 电芯温度 88°C */
    battery_gas_sensor_sample_t s = {.co_ppm = 420.0,
                                     .h2_ppm = 850.0,
                                     .voc_ppb = 15000.0,
                                     .pressure_pa = 101500.0,
                                     .smoke_density = 0.35,
                                     .max_cell_temp_c = 88.0,
                                     .min_cell_volt_v = 2.10,
                                     .timestamp_s = 30.0};
    battery_gas_ew_push_sample(&ew, &s);

    int rc = battery_gas_ew_process_step(&ew, 30.0);
    assert_int_equal(rc, 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_STAGE3_RUNAWAY_FIRE);
    assert_true(rep->prob_runaway_fire >= 0.50);
    assert_true(rep->trip_contactor_req);
    assert_true(rep->trigger_extinguisher_req); /* 必须释放灭火剂 */
    assert_true(rep->estimated_seconds_to_runaway <= 60.0);
}

/* 6. 测试传感器断线/量程越界故障处理 */
static void test_sensor_fault_detection(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 异常气压值 (5000 Pa 远低于正常大气压下限 60000 Pa) */
    battery_gas_sensor_sample_t broken = {.co_ppm = 0.0,
                                          .h2_ppm = 0.0,
                                          .voc_ppb = 0.0,
                                          .pressure_pa = 5000.0,
                                          .smoke_density = 0.0,
                                          .max_cell_temp_c = 25.0,
                                          .min_cell_volt_v = 3.30,
                                          .timestamp_s = 10.0};
    battery_gas_ew_push_sample(&ew, &broken);

    int rc = battery_gas_ew_process_step(&ew, 10.0);
    assert_int_equal(rc, 0);

    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_SENSOR_FAULT);
    assert_false(rep->trigger_extinguisher_req);
}

/* 7. 测试重置与基线恢复 */
static void test_reset_and_recovery(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    battery_gas_sensor_sample_t s = {.co_ppm = 100.0,
                                     .h2_ppm = 200.0,
                                     .voc_ppb = 500.0,
                                     .pressure_pa = 101325.0,
                                     .smoke_density = 0.05,
                                     .max_cell_temp_c = 50.0,
                                     .min_cell_volt_v = 3.20,
                                     .timestamp_s = 10.0};
    battery_gas_ew_push_sample(&ew, &s);
    battery_gas_ew_process_step(&ew, 10.0);
    assert_true(ew.report.valid);

    battery_gas_ew_reset(&ew);
    assert_false(ew.report.valid);
    assert_int_equal(ew.sample_count, 0);
    assert_null(battery_gas_ew_get_report(&ew));
}

/* 8. 测试 NaN 脏数据注入与阈值除零防御 */
static void test_nan_injection_and_zero_division_defense(void **state) {
    (void)state;
    battery_gas_ew_detector_t ew;
    battery_gas_ew_init(&ew, NULL);

    /* 8.1 注入 NaN 气压值，断言 IEEE 754 比较不绕过并安全判定为 SENSOR_FAULT */
    battery_gas_sensor_sample_t nan_sample = {.co_ppm = 1.0,
                                              .h2_ppm = 2.0,
                                              .voc_ppb = 50.0,
                                              .pressure_pa = NAN, /* 异常 NaN 气压 */
                                              .smoke_density = 0.01,
                                              .max_cell_temp_c = 25.0,
                                              .min_cell_volt_v = 3.30,
                                              .timestamp_s = 10.0};
    battery_gas_ew_push_sample(&ew, &nan_sample);
    int rc = battery_gas_ew_process_step(&ew, 10.0);
    assert_int_equal(rc, 0);
    const battery_gas_ew_report_t *rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_int_equal(rep->stage, BATTERY_GAS_STAGE_SENSOR_FAULT);
    assert_false(rep->trigger_extinguisher_req);
    assert_true(isfinite(ew.baseline_pressure));

    /* 8.2 测试温升阈值 temp_runaway_c == 55.0 临界除零配置防御 */
    battery_gas_ew_config_t edge_cfg = {.temp_runaway_c = 55.0, /* 与基准温差 55.0 完全重合 */
                                        .voc_trace_ppb = 0.0, /* 0 阈值 */
                                        .co_vent_ppm = 0.0,
                                        .h2_vent_ppm = 0.0};
    assert_int_equal(battery_gas_ew_init(&ew, &edge_cfg), 0);
    assert_true(ew.config.temp_runaway_c > 55.0); /* 必须修正或有保护 */

    battery_gas_sensor_sample_t valid_sample = {.co_ppm = 10.0,
                                                .h2_ppm = 20.0,
                                                .voc_ppb = 100.0,
                                                .pressure_pa = 101325.0,
                                                .smoke_density = 0.05,
                                                .max_cell_temp_c = 60.0,
                                                .min_cell_volt_v = 3.30,
                                                .timestamp_s = 20.0};
    battery_gas_ew_push_sample(&ew, &valid_sample);
    rc = battery_gas_ew_process_step(&ew, 20.0);
    assert_int_equal(rc, 0);
    rep = battery_gas_ew_get_report(&ew);
    assert_non_null(rep);
    assert_true(isfinite(rep->prob_prevent_latent));
    assert_true(isfinite(rep->prob_vent_confirmed));
    assert_true(isfinite(rep->prob_runaway_fire));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ew_init_and_config),
        cmocka_unit_test(test_normal_background_safe),
        cmocka_unit_test(test_stage1_prevent_trace_detection),
        cmocka_unit_test(test_stage2_venting_pulse_confirmation),
        cmocka_unit_test(test_stage3_runaway_fire_imminent),
        cmocka_unit_test(test_sensor_fault_detection),
        cmocka_unit_test(test_reset_and_recovery),
        cmocka_unit_test(test_nan_injection_and_zero_division_defense),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
