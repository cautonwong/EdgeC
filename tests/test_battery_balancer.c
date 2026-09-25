#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "battery_balancer.h"

/* 1. 测试初始化与配置校验 */
static void test_balancer_init_and_config(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = (battery_balancer_t *)store.raw;

    /* 默认初始化 16 电芯 */
    bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);
    assert_int_equal(battery_balancer_get_cell_count(bal), 16);
    const battery_bal_config_t *cfg = battery_balancer_get_config(bal);
    assert_non_null(cfg);
    assert_int_equal(cfg->mode, BATTERY_BAL_MODE_ACTIVE_PAIRWISE);
    assert_double_equal(cfg->start_voltage_diff_mv, 15.0, 0.01);
    assert_double_equal(cfg->stop_voltage_diff_mv, 5.0, 0.01);

    /* 自定义配置 */
    battery_bal_config_t custom_cfg = {.mode = BATTERY_BAL_MODE_PASSIVE,
                                       .cell_count = 24,
                                       .nominal_capacity_ah = 314.0,
                                       .start_voltage_diff_mv = 20.0,
                                       .stop_voltage_diff_mv = 8.0,
                                       .start_soc_diff_pct = 2.0,
                                       .stop_soc_diff_pct = 0.8,
                                       .max_active_current_a = 5.0,
                                       .bleed_resistor_ohm = 25.0,
                                       .max_concurrent_channels = 6,
                                       .adjacent_interlock = true,
                                       .temp_min_c = 10.0,
                                       .temp_max_c = 40.0,
                                       .cell_min_volt_v = 2.90,
                                       .cell_max_volt_v = 3.60};
    bal = battery_balancer_create_in(&store, &custom_cfg, 24, NULL);
    assert_non_null(bal);
    assert_int_equal(battery_balancer_get_cell_count(bal), 24);
    cfg = battery_balancer_get_config(bal);
    assert_non_null(cfg);
    assert_int_equal(cfg->mode, BATTERY_BAL_MODE_PASSIVE);
    assert_double_equal(cfg->nominal_capacity_ah, 314.0, 0.01);
}

/* 2. 测试整组一致时自动闲置旁路 */
static void test_balanced_pack_idle_standby(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);

    /* 16 串电芯电压高度一致 (3.300V, SOC 0.50) */
    for (uint16_t i = 0; i < 16; i++) {
        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = 3.300 + 0.001 * (i % 2),
                                          .soc = 0.500 + 0.001 * (i % 2),
                                          .temperature_c = 25.0,
                                          .soh = 1.0,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_false(plan->is_balancing_active);
    assert_int_equal(plan->active_channels_count, 0);
    for (uint16_t i = 0; i < 16; i++) {
        assert_int_equal(plan->commands[i].action, BATTERY_BAL_ACTION_IDLE);
    }
}

/* 3. 测试被动阻性放电均衡 */
static void test_passive_balancing_discharge(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_bal_config_t cfg = {.mode = BATTERY_BAL_MODE_PASSIVE,
                                .cell_count = 16,
                                .nominal_capacity_ah = 280.0,
                                .start_voltage_diff_mv = 15.0,
                                .stop_voltage_diff_mv = 5.0,
                                .start_soc_diff_pct = 1.5,
                                .stop_soc_diff_pct = 0.5,
                                .bleed_resistor_ohm = 33.0,
                                .max_concurrent_channels = 4,
                                .adjacent_interlock = false,
                                .temp_min_c = 5.0,
                                .temp_max_c = 45.0,
                                .cell_min_volt_v = 2.80,
                                .cell_max_volt_v = 3.65};
    battery_balancer_t *bal = battery_balancer_create_in(&store, &cfg, 16, NULL);
    assert_non_null(bal);

    /* 设置 #2 电芯电压偏高 3.340V，其余 3.300V */
    for (uint16_t i = 0; i < 16; i++) {
        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = (i == 2) ? 3.340 : 3.300,
                                          .soc = (i == 2) ? 0.58 : 0.50,
                                          .temperature_c = 25.0,
                                          .soh = 1.0,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_true(plan->is_balancing_active);
    assert_true(plan->voltage_diff_mv >= 38.0);
    assert_int_equal(plan->commands[2].action, BATTERY_BAL_ACTION_DISCHARGE_PASSIVE);
    assert_true(plan->commands[2].duty_cycle > 0.5);
    assert_true(plan->commands[2].target_current_a < 0.0);
    assert_true(plan->commands[2].power_dissipated_w > 0.1);
}

/* 4. 测试双向主动电荷转移与高效配对 */
static void test_active_pairwise_bidirectional_transfer(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);

    /* 设置 #0 电芯 SOC 0.65 (高)，#10 电芯 SOC 0.35 (低)，其余 0.50 */
    for (uint16_t i = 0; i < 16; i++) {
        double s = 0.50;
        double v = 3.300;
        if (i == 0) {
            s = 0.65;
            v = 3.360;
        }
        if (i == 10) {
            s = 0.35;
            v = 3.240;
        }

        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = v,
                                          .soc = s,
                                          .temperature_c = 25.0,
                                          .soh = 0.98,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_true(plan->is_balancing_active);
    assert_true(plan->active_pairs_count >= 1);

    /* #0 作为放电源，#10 作为充电宿 */
    assert_int_equal(plan->commands[0].action, BATTERY_BAL_ACTION_ACTIVE_SOURCE);
    assert_true(plan->commands[0].target_current_a < 0.0);

    assert_int_equal(plan->commands[10].action, BATTERY_BAL_ACTION_ACTIVE_TARGET);
    assert_true(plan->commands[10].target_current_a > 0.0);

    /* 校验配对 */
    assert_int_equal(plan->pairs[0].source_cell_id, 0);
    assert_int_equal(plan->pairs[0].target_cell_id, 10);
    assert_true(plan->pairs[0].efficiency >= 0.85);
    assert_true(plan->energy_efficiency_score >= 0.80);
}

/* 5. 测试温度与电压安全边界互锁 */
static void test_temperature_and_voltage_safety_interlock(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);

    /* #4 电芯高 SOC 但温度过热 52°C (> 45°C) */
    /* #8 电芯低 SOC 但欠压 2.70V (< 2.80V) */
    for (uint16_t i = 0; i < 16; i++) {
        double s = 0.50;
        double v = 3.300;
        double t = 25.0;
        if (i == 4) {
            s = 0.70;
            v = 3.380;
            t = 52.0;
        }
        if (i == 8) {
            s = 0.30;
            v = 2.700;
            t = 25.0;
        }

        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = v,
                                          .soc = s,
                                          .temperature_c = t,
                                          .soh = 1.0,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    /* 校验超温与欠压电芯被互锁抑制 */
    assert_true(plan->commands[4].interlocked);
    assert_int_equal(plan->commands[4].action, BATTERY_BAL_ACTION_IDLE);

    assert_true(plan->commands[8].interlocked);
    assert_int_equal(plan->commands[8].action, BATTERY_BAL_ACTION_IDLE);
}

/* 6. 测试相邻电芯硬件互锁 */
static void test_adjacent_hardware_interlock(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_bal_config_t cfg = {.mode = BATTERY_BAL_MODE_PASSIVE,
                                .cell_count = 16,
                                .nominal_capacity_ah = 280.0,
                                .start_voltage_diff_mv = 10.0,
                                .stop_voltage_diff_mv = 5.0,
                                .start_soc_diff_pct = 1.0,
                                .stop_soc_diff_pct = 0.5,
                                .bleed_resistor_ohm = 33.0,
                                .max_concurrent_channels = 4,
                                .adjacent_interlock = true, /* 开启相邻互锁 */
                                .temp_min_c = 5.0,
                                .temp_max_c = 45.0,
                                .cell_min_volt_v = 2.80,
                                .cell_max_volt_v = 3.65};
    battery_balancer_t *bal = battery_balancer_create_in(&store, &cfg, 16, NULL);
    assert_non_null(bal);

    /* #2 和 #3 均偏高 */
    for (uint16_t i = 0; i < 16; i++) {
        double v = (i == 2 || i == 3) ? 3.350 : 3.300;
        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = v,
                                          .soc = 0.50,
                                          .temperature_c = 25.0,
                                          .soh = 1.0,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    /* #2 激活放电，则相邻 #3 应被互锁抑制 */
    assert_int_equal(plan->commands[2].action, BATTERY_BAL_ACTION_DISCHARGE_PASSIVE);
    assert_true(plan->commands[3].interlocked);
    assert_int_equal(plan->commands[3].action, BATTERY_BAL_ACTION_IDLE);
}

/* 7. 测试均衡时间预测与重置 */
static void test_energy_time_trajectory_and_reset(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);

    /* 设置明显不均衡 */
    battery_bal_cell_input_t high = {.cell_id = 0,
                                     .voltage_v = 3.40,
                                     .soc = 0.70,
                                     .temperature_c = 25.0,
                                     .soh = 1.0,
                                     .r0_mohm = 0.5};
    battery_bal_cell_input_t low = {.cell_id = 12,
                                    .voltage_v = 3.20,
                                    .soc = 0.30,
                                    .temperature_c = 25.0,
                                    .soh = 1.0,
                                    .r0_mohm = 0.5};
    battery_balancer_update_cell(bal, 0, &high);
    battery_balancer_update_cell(bal, 12, &low);

    battery_balancer_plan_step(bal, 100.0);
    const battery_bal_plan_t *p1 = battery_balancer_get_plan(bal);
    assert_non_null(p1);
    assert_true(p1->estimated_time_to_balance_s > 0.0);

    /* 步进 10 秒后累计能量增加 */
    battery_balancer_plan_step(bal, 110.0);
    assert_true(battery_balancer_get_transferred_wh(bal) > 0.0);

    /* 重置后应清零 */
    battery_balancer_reset(bal);
    assert_double_equal(battery_balancer_get_transferred_wh(bal), 0.0, 0.001);
    assert_null(battery_balancer_get_plan(bal));
}

/* 8. 测试 NaN 脏数据注入与 qsort 排序弱序违规整改防御 */
static void test_nan_cell_defense_and_qsort_weak_order(void **state) {
    (void)state;
    battery_balancer_storage_t store;
    battery_balancer_t *bal = battery_balancer_create_in(&store, NULL, 16, NULL);
    assert_non_null(bal);

    /* 注入包含 NaN 的电芯数据 (#3 电芯 SOC 为 NaN, #7 电芯电压为 NaN) */
    for (uint16_t i = 0; i < 16; i++) {
        double s = (i % 2 == 0) ? 0.65 : 0.45;
        double v = (i % 2 == 0) ? 3.35 : 3.25;
        if (i == 3) s = NAN;
        if (i == 7) v = NAN;

        battery_bal_cell_input_t input = {.cell_id = i,
                                          .voltage_v = v,
                                          .soc = s,
                                          .temperature_c = 25.0,
                                          .soh = 0.95,
                                          .r0_mohm = 0.50};
        battery_balancer_update_cell(bal, i, &input);
    }

    /* 执行规划步进，断言 qsort 绝不因 NaN 崩溃，且极差统计不出现 -2e12 mV 异常 */
    int rc = battery_balancer_plan_step(bal, 100.0);
    assert_int_equal(rc, 0);

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    assert_non_null(plan);
    assert_true(isfinite(plan->voltage_diff_mv));
    assert_true(plan->voltage_diff_mv >= 0.0);
    assert_true(plan->voltage_diff_mv < 1000.0); /* 正常极差 ~100mV */
    assert_true(isfinite(plan->avg_soc));
    assert_true(isfinite(plan->max_soc_diff_pct));

    /* 校验 NaN 电芯已被互锁隔离 */
    assert_true(plan->commands[3].interlocked);
    assert_int_equal(plan->commands[3].action, BATTERY_BAL_ACTION_IDLE);
    assert_true(plan->commands[7].interlocked);
    assert_int_equal(plan->commands[7].action, BATTERY_BAL_ACTION_IDLE);

    /* 8.2 校验非法 start_soc_diff_pct 初始配置自愈防除零 */
    battery_bal_config_t edge_cfg = {.cell_count = 16,
                                     .start_soc_diff_pct = 0.0,
                                     .stop_soc_diff_pct = 0.0,
                                     .max_active_current_a = 5.0};
    bal = battery_balancer_create_in(&store, &edge_cfg, 16, NULL);
    assert_non_null(bal);
    const battery_bal_config_t *cfg = battery_balancer_get_config(bal);
    assert_non_null(cfg);
    assert_true(cfg->start_soc_diff_pct > 0.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_balancer_init_and_config),
        cmocka_unit_test(test_balanced_pack_idle_standby),
        cmocka_unit_test(test_passive_balancing_discharge),
        cmocka_unit_test(test_active_pairwise_bidirectional_transfer),
        cmocka_unit_test(test_temperature_and_voltage_safety_interlock),
        cmocka_unit_test(test_adjacent_hardware_interlock),
        cmocka_unit_test(test_energy_time_trajectory_and_reset),
        cmocka_unit_test(test_nan_cell_defense_and_qsort_weak_order),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
