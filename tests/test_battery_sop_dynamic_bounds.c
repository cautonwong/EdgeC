/**
 * @file test_battery_sop_dynamic_bounds.c
 * @brief 电池峰值充放电功率 (SOP) 多约束物理边界求解器单元测试 (AEP-740 / Issue #270)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "battery_sop_dynamic_bounds.h"

#define TEST_CAPACITY_AH 100.0

static void test_sop_spec_initialization(void **state) {
    (void)state;
    battery_sop_cell_spec_t lfp_spec, nmc_spec, na_spec;

    assert_true(battery_sop_spec_init_default(&lfp_spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));
    assert_int_equal(lfp_spec.chem_type, BATTERY_CHEM_LFP);
    assert_float_equal((float)lfp_spec.v_min, 2.50f, 0.001f);
    assert_float_equal((float)lfp_spec.v_max, 3.65f, 0.001f);
    assert_true(lfp_spec.r0_ref_ohm > 0.0);
    assert_true(lfp_spec.max_charge_current_a > 0.0);

    assert_true(battery_sop_spec_init_default(&nmc_spec, BATTERY_CHEM_NMC, TEST_CAPACITY_AH));
    assert_int_equal(nmc_spec.chem_type, BATTERY_CHEM_NMC);
    assert_float_equal((float)nmc_spec.v_min, 2.80f, 0.001f);
    assert_float_equal((float)nmc_spec.v_max, 4.20f, 0.001f);

    assert_true(battery_sop_spec_init_default(&na_spec, BATTERY_CHEM_SODIUM_ION, TEST_CAPACITY_AH));
    assert_int_equal(na_spec.chem_type, BATTERY_CHEM_SODIUM_ION);
    assert_float_equal((float)na_spec.v_min, 1.80f, 0.001f);
    assert_float_equal((float)na_spec.v_max, 3.90f, 0.001f);

    /* 边界防御测试 */
    assert_false(battery_sop_spec_init_default(NULL, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));
    assert_false(battery_sop_spec_init_default(&lfp_spec, BATTERY_CHEM_LFP, -10.0));
}

static void test_sop_voltage_upper_lower_bound_enforcement(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    /* 1. 高 SOC (0.93) 且存在正极化过电位充电场景：受限于 V_max
     * (3.65V)，充电电流由电压硬边界主导截断 */
    battery_sop_cell_state_t high_soc_state = {.soc = 0.93,
                                               .up_polarization_v = 0.035,
                                               .cell_temp_c = 25.0,
                                               .terminal_voltage_meas_v = 3.55,
                                               .current_meas_a = 0.0};

    battery_sop_result_t res_high;
    assert_true(battery_sop_solve_single_horizon(&spec, &high_soc_state, 10.0, &res_high));
    assert_true(res_high.max_charge_current_a > 0.0);
    assert_true(res_high.pred_terminal_v_charge <= spec.v_max + 1e-6);
    assert_int_equal(res_high.charge_limit_reason, SOP_LIMIT_VOLTAGE_UPPER);

    /* 2. 低 SOC (0.08) 三元锂放电场景：受限于 V_min (2.80V)，放电电流由下限电压硬边界截断 */
    battery_sop_cell_spec_t nmc_spec;
    assert_true(battery_sop_spec_init_default(&nmc_spec, BATTERY_CHEM_NMC, TEST_CAPACITY_AH));
    battery_sop_cell_state_t low_soc_state = {.soc = 0.08,
                                              .up_polarization_v = -0.020,
                                              .cell_temp_c = 25.0,
                                              .terminal_voltage_meas_v = 2.85,
                                              .current_meas_a = 0.0};

    battery_sop_result_t res_low;
    assert_true(battery_sop_solve_single_horizon(&nmc_spec, &low_soc_state, 10.0, &res_low));
    assert_true(res_low.max_discharge_current_a > 0.0);
    assert_true(res_low.pred_terminal_v_discharge >= nmc_spec.v_min - 1e-6);
    assert_int_equal(res_low.discharge_limit_reason, SOP_LIMIT_VOLTAGE_LOWER);
}

static void test_sop_soc_boundary_cutoff(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    /* 1. 已满电 (SOC = 0.95 >= soc_max)，禁止充电 */
    battery_sop_cell_state_t full_state = {.soc = 0.95,
                                           .up_polarization_v = 0.0,
                                           .cell_temp_c = 25.0,
                                           .terminal_voltage_meas_v = 3.45,
                                           .current_meas_a = 0.0};
    battery_sop_result_t res_full;
    assert_true(battery_sop_solve_single_horizon(&spec, &full_state, 10.0, &res_full));
    assert_float_equal((float)res_full.max_charge_current_a, 0.0f, 0.001f);
    assert_float_equal((float)res_full.max_charge_power_w, 0.0f, 0.001f);
    assert_int_equal(res_full.charge_limit_reason, SOP_LIMIT_SOC_UPPER);
    /* 但允许放电 */
    assert_true(res_full.max_discharge_current_a > 50.0);

    /* 2. 已放空 (SOC = 0.05 <= soc_min)，禁止放电 */
    battery_sop_cell_state_t empty_state = {.soc = 0.05,
                                            .up_polarization_v = 0.0,
                                            .cell_temp_c = 25.0,
                                            .terminal_voltage_meas_v = 2.65,
                                            .current_meas_a = 0.0};
    battery_sop_result_t res_empty;
    assert_true(battery_sop_solve_single_horizon(&spec, &empty_state, 10.0, &res_empty));
    assert_float_equal((float)res_empty.max_discharge_current_a, 0.0f, 0.001f);
    assert_float_equal((float)res_empty.max_discharge_power_w, 0.0f, 0.001f);
    assert_int_equal(res_empty.discharge_limit_reason, SOP_LIMIT_SOC_LOWER);
    /* 但允许充电 */
    assert_true(res_empty.max_charge_current_a > 50.0);
}

static void test_sop_cold_thermal_lithium_plating_derating(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    /* 1. 0°C 以下：强制禁止充电以防止析锂 */
    battery_sop_cell_state_t frozen_state = {.soc = 0.50,
                                             .up_polarization_v = 0.0,
                                             .cell_temp_c = -2.0,
                                             .terminal_voltage_meas_v = 3.25,
                                             .current_meas_a = 0.0};
    battery_sop_result_t res_frozen;
    assert_true(battery_sop_solve_single_horizon(&spec, &frozen_state, 10.0, &res_frozen));
    assert_float_equal((float)res_frozen.max_charge_current_a, 0.0f, 0.001f);
    assert_int_equal(res_frozen.charge_limit_reason, SOP_LIMIT_THERMAL_COLD);

    /* 2. 7.5°C：在 0°C~15°C 降额区间，充电电流应线性降额至约 50% */
    battery_sop_cell_state_t cold_state = {.soc = 0.50,
                                           .up_polarization_v = 0.0,
                                           .cell_temp_c = 7.5,
                                           .terminal_voltage_meas_v = 3.25,
                                           .current_meas_a = 0.0};
    battery_sop_result_t res_cold;
    assert_true(battery_sop_solve_single_horizon(&spec, &cold_state, 10.0, &res_cold));
    assert_true(res_cold.max_charge_current_a > 0.0);
    assert_true(res_cold.max_charge_current_a <= spec.max_charge_current_a * 0.55);
    assert_int_equal(res_cold.charge_limit_reason, SOP_LIMIT_THERMAL_COLD);
}

static void test_sop_hot_thermal_overheating_derating(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    /* 56°C >= 55°C 切断阈值：充放电双向强制切断归零 */
    battery_sop_cell_state_t overheat_state = {.soc = 0.50,
                                               .up_polarization_v = 0.0,
                                               .cell_temp_c = 56.0,
                                               .terminal_voltage_meas_v = 3.25,
                                               .current_meas_a = 0.0};
    battery_sop_result_t res_hot;
    assert_true(battery_sop_solve_single_horizon(&spec, &overheat_state, 10.0, &res_hot));
    assert_float_equal((float)res_hot.max_charge_current_a, 0.0f, 0.001f);
    assert_float_equal((float)res_hot.max_discharge_current_a, 0.0f, 0.001f);
    assert_int_equal(res_hot.charge_limit_reason, SOP_LIMIT_THERMAL_HOT);
    assert_int_equal(res_hot.discharge_limit_reason, SOP_LIMIT_THERMAL_HOT);
}

static void test_sop_multi_time_scale_monotonicity(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    /* 1. 验证常规工况下充放电极限电流随时间窗口扩大单调非增 (I_1s >= I_10s >= I_30s >= I_60s) */
    battery_sop_cell_state_t normal_state = {.soc = 0.80,
                                             .up_polarization_v = 0.01,
                                             .cell_temp_c = 25.0,
                                             .terminal_voltage_meas_v = 3.35,
                                             .current_meas_a = 0.0};

    double horizons[4] = {1.0, 10.0, 30.0, 60.0};
    battery_sop_multi_horizon_result_t multi_res;
    assert_true(battery_sop_solve_multi_horizons(&spec, &normal_state, horizons, 4, &multi_res));
    assert_int_equal(multi_res.num_horizons, 4);

    for (size_t i = 0; i < 3; i++) {
        assert_true(multi_res.horizons[i].max_charge_current_a >=
                    multi_res.horizons[i + 1].max_charge_current_a - 1e-6);
        assert_true(multi_res.horizons[i].max_discharge_current_a >=
                    multi_res.horizons[i + 1].max_discharge_current_a - 1e-6);
    }

    /* 2. 在电压硬约束主导区间 (SOC = 0.94 高极化)，验证允许峰值充电功率单调递减 (P_1s >= P_10s >=
     * P_30s >= P_60s) */
    battery_sop_cell_state_t high_volt_state = {.soc = 0.94,
                                                .up_polarization_v = 0.02,
                                                .cell_temp_c = 25.0,
                                                .terminal_voltage_meas_v = 3.56,
                                                .current_meas_a = 0.0};
    battery_sop_multi_horizon_result_t volt_res;
    assert_true(battery_sop_solve_multi_horizons(&spec, &high_volt_state, horizons, 4, &volt_res));
    for (size_t i = 0; i < 3; i++) {
        assert_true(volt_res.horizons[i].max_charge_power_w >=
                    volt_res.horizons[i + 1].max_charge_power_w - 1e-6);
    }
}

static void test_sop_pack_scaling_series_parallel(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    assert_true(battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH));

    battery_sop_cell_state_t normal_state = {.soc = 0.50,
                                             .up_polarization_v = 0.0,
                                             .cell_temp_c = 25.0,
                                             .terminal_voltage_meas_v = 3.28,
                                             .current_meas_a = 0.0};

    battery_sop_result_t cell_res;
    assert_true(battery_sop_solve_single_horizon(&spec, &normal_state, 10.0, &cell_res));

    /* 缩放至 240S2P 电池簇 (240串 2并) */
    battery_sop_result_t pack_res;
    assert_true(battery_sop_scale_to_pack(&cell_res, 240, 2, &pack_res));

    assert_float_equal((float)pack_res.max_charge_current_a,
                       (float)(cell_res.max_charge_current_a * 2.0), 0.01f);
    assert_float_equal((float)pack_res.max_discharge_current_a,
                       (float)(cell_res.max_discharge_current_a * 2.0), 0.01f);
    assert_float_equal((float)pack_res.pred_terminal_v_charge,
                       (float)(cell_res.pred_terminal_v_charge * 240.0), 0.1f);
    assert_float_equal((float)pack_res.pred_terminal_v_discharge,
                       (float)(cell_res.pred_terminal_v_discharge * 240.0), 0.1f);
    assert_float_equal((float)pack_res.max_charge_power_w,
                       (float)(cell_res.max_charge_power_w * 480.0), 1.0f);
    assert_float_equal((float)pack_res.max_discharge_power_w,
                       (float)(cell_res.max_discharge_power_w * 480.0), 1.0f);
}

static void test_sop_invalid_inputs_safety(void **state) {
    (void)state;
    battery_sop_cell_spec_t spec;
    battery_sop_spec_init_default(&spec, BATTERY_CHEM_LFP, TEST_CAPACITY_AH);
    battery_sop_cell_state_t st = {.soc = 0.5, .cell_temp_c = 25.0};
    battery_sop_result_t res;

    assert_false(battery_sop_solve_single_horizon(NULL, &st, 1.0, &res));
    assert_false(battery_sop_solve_single_horizon(&spec, NULL, 1.0, &res));
    assert_false(battery_sop_solve_single_horizon(&spec, &st, -1.0, &res));
    assert_false(battery_sop_solve_single_horizon(&spec, &st, 1.0, NULL));

    assert_false(battery_sop_scale_to_pack(NULL, 100, 1, &res));
    assert_false(battery_sop_scale_to_pack(&res, 0, 1, &res));
    assert_false(battery_sop_scale_to_pack(&res, 100, 0, &res));

    /* 验证限制因子字符串不返回 NULL */
    assert_non_null(battery_sop_limit_factor_str(SOP_LIMIT_NONE));
    assert_non_null(battery_sop_limit_factor_str(SOP_LIMIT_VOLTAGE_UPPER));
    assert_non_null(battery_sop_limit_factor_str(SOP_LIMIT_THERMAL_COLD));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sop_spec_initialization),
        cmocka_unit_test(test_sop_voltage_upper_lower_bound_enforcement),
        cmocka_unit_test(test_sop_soc_boundary_cutoff),
        cmocka_unit_test(test_sop_cold_thermal_lithium_plating_derating),
        cmocka_unit_test(test_sop_hot_thermal_overheating_derating),
        cmocka_unit_test(test_sop_multi_time_scale_monotonicity),
        cmocka_unit_test(test_sop_pack_scaling_series_parallel),
        cmocka_unit_test(test_sop_invalid_inputs_safety),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
