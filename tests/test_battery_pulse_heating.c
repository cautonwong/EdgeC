/**
 * @file test_battery_pulse_heating.c
 * @brief 极端低温环境电池脉冲自加热与热电耦合控制算法单元测试 (AEP-738)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>
#include <math.h>
#include <string.h>

#include "battery_pulse_heating.h"

static void test_pulse_heat_init_and_config(void **state) {
    (void)state;
    pulse_heat_engine_t engine;

    /* 默认配置初始化 */
    int ret = pulse_heat_engine_init(&engine, NULL);
    assert_int_equal(ret, 0);
    assert_int_equal(engine.current_state, PULSE_HEAT_STATE_IDLE);
    assert_int_equal(engine.active_fault, PULSE_HEAT_FAULT_NONE);
    assert_float_equal(engine.cfg.target_temp_c, 10.0f, 0.001f);
    assert_float_equal(engine.cfg.nominal_frequency_hz, 500.0f, 0.001f);

    assert_string_equal(pulse_heat_state_to_str(PULSE_HEAT_STATE_IDLE), "IDLE");
    assert_string_equal(pulse_heat_state_to_str(PULSE_HEAT_STATE_ACTIVE_HEATING), "ACTIVE_HEATING");
    assert_string_equal(pulse_heat_fault_to_str(PULSE_HEAT_FAULT_NONE), "NONE");
    assert_string_equal(pulse_heat_fault_to_str(PULSE_HEAT_FAULT_LITHIUM_PLATING_RISK), "LITHIUM_PLATING_RISK");

    /* 自定义配置初始化 */
    pulse_heat_config_t custom_cfg = {
        .target_temp_c = 15.0f,
        .min_start_temp_c = -5.0f,
        .max_temp_limit_c = 50.0f,
        .max_thermal_gradient_c = 8.0f,
        .max_cell_temp_spread_c = 5.0f,
        .min_soc = 0.20f,
        .max_soc = 0.80f,
        .max_current_amp_a = 200.0f,
        .nominal_frequency_hz = 600.0f,
        .sweep_freq_start_hz = 150.0f,
        .sweep_freq_end_hz = 1200.0f,
        .current_ramp_rate_a_per_s = 40.0f,
        .active_burst_duration_s = 25.0f,
        .homogenize_duration_s = 5.0f,
        .timeout_limit_s = 800.0f,
        .cell_v_min_nominal = 2.1f,
        .cell_v_max_nominal = 3.7f,
        .cell_heat_capacity_j_per_k = 500.0f,
        .core_surface_r_th = 0.9f,
        .surface_ambient_r_th = 3.0f,
        .baseline_resistance_ohm = 0.0015f,
        .arrhenius_activation_energy_j = 25000.0f
    };

    ret = pulse_heat_engine_init(&engine, &custom_cfg);
    assert_int_equal(ret, 0);
    assert_float_equal(engine.cfg.target_temp_c, 15.0f, 0.001f);
    assert_float_equal(engine.cfg.max_current_amp_a, 200.0f, 0.001f);
}

static void test_pulse_heat_precheck_safety_gates(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = -20.0f,
        .pack_soc = 0.50f,
        .inverter_ready = false, /* 逆变器未就绪 */
        .actual_current_rms_a = 0.0f,
        .actual_frequency_hz = 0.0f
    };
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.20f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -20.0f;

    /* 启动加热 */
    int ret = pulse_heat_engine_start(&engine);
    assert_int_equal(ret, 0);

    pulse_heat_control_output_t out;
    ret = pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(ret, 0);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FAULT);
    assert_int_equal(out.fault, PULSE_HEAT_FAULT_INVERTER_NOT_READY);
    assert_false(out.inverter_enable);

    /* 重置并测试 SOC 过低门禁 */
    pulse_heat_engine_reset(&engine);
    telem.inverter_ready = true;
    telem.pack_soc = 0.10f; /* 低于 0.15 */
    pulse_heat_engine_start(&engine);
    ret = pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(ret, 0);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FAULT);
    assert_int_equal(out.fault, PULSE_HEAT_FAULT_SOC_OUT_OF_BOUNDS);

    /* 重置并测试电芯间温差过大门禁 */
    pulse_heat_engine_reset(&engine);
    telem.pack_soc = 0.60f;
    telem.cell_temps[0] = -20.0f;
    telem.cell_temps[1] = -14.0f; /* 极差 6℃ > 4℃ */
    pulse_heat_engine_start(&engine);
    ret = pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(ret, 0);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FAULT);
    assert_int_equal(out.fault, PULSE_HEAT_FAULT_HIGH_THERMAL_GRADIENT);
}

static void test_pulse_heat_frequency_sweep_and_ramp(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = -20.0f,
        .pack_soc = 0.60f,
        .inverter_ready = true,
        .actual_current_rms_a = 0.0f,
        .actual_frequency_hz = 0.0f
    };
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.20f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -20.0f;

    pulse_heat_engine_start(&engine);
    pulse_heat_control_output_t out;

    /* 步进第 1 周期: PRECHECK -> FREQ_SWEEP */
    pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FREQ_SWEEP);
    assert_float_equal(out.target_current_amp_a, 10.0f, 0.01f);
    assert_true(out.inverter_enable);

    /* 连续步进 1.0s 完成扫频 -> RAMP_UP */
    for (int i = 0; i < 10; i++) {
        pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    }
    assert_int_equal(out.state, PULSE_HEAT_STATE_RAMP_UP);
    assert_float_equal(out.target_frequency_hz, 500.0f, 0.01f);

    /* 验证斜坡电流上升：30 A/s 速率 */
    float prev_curr = out.target_current_amp_a;
    for (int i = 0; i < 10; i++) {
        pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
        assert_true(out.target_current_amp_a >= prev_curr);
        prev_curr = out.target_current_amp_a;
    }

    /* 步进直至达到稳态 150A -> 转入 ACTIVE_HEATING */
    while (out.state == PULSE_HEAT_STATE_RAMP_UP) {
        pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    }
    assert_int_equal(out.state, PULSE_HEAT_STATE_ACTIVE_HEATING);
    assert_float_equal(out.target_current_amp_a, 150.0f, 0.01f);
}

static void test_pulse_heat_full_heating_cycle_convergence(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = -5.0f,
        .pack_soc = 0.65f,
        .inverter_ready = true,
        .actual_current_rms_a = 0.0f,
        .actual_frequency_hz = 500.0f
    };
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.25f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -10.0f;

    pulse_heat_engine_start(&engine);
    pulse_heat_control_output_t out;

    bool seen_sweep = false;
    bool seen_ramp = false;
    bool seen_active = false;
    bool seen_homogenize = false;
    bool completed = false;

    /* 模拟闭环运行仿真，步长 0.2s */
    for (int step = 0; step < 2500; step++) {
        /* 电芯表面温度随核心温度和热阻传导缓慢上升 */
        if (out.state == PULSE_HEAT_STATE_ACTIVE_HEATING) {
            seen_active = true;
            telem.actual_current_rms_a = out.target_current_amp_a * 0.7071f;
        } else if (out.state == PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE) {
            seen_homogenize = true;
            telem.actual_current_rms_a = 0.0f;
        } else if (out.state == PULSE_HEAT_STATE_FREQ_SWEEP) {
            seen_sweep = true;
            telem.actual_current_rms_a = out.target_current_amp_a * 0.7071f;
        } else if (out.state == PULSE_HEAT_STATE_RAMP_UP) {
            seen_ramp = true;
            telem.actual_current_rms_a = out.target_current_amp_a * 0.7071f;
        }

        pulse_heat_engine_step(&engine, &telem, 0.2f, &out);

        /* 同步仿真表面传感器读数跟踪估算表面温度 */
        for (int i = 0; i < 4; i++) {
            telem.cell_temps[i] = out.estimated_surface_temp_c;
        }

        if (out.state == PULSE_HEAT_STATE_COMPLETED) {
            completed = true;
            break;
        }
    }

    assert_true(seen_sweep);
    assert_true(seen_ramp);
    assert_true(seen_active);
    assert_true(seen_homogenize);
    assert_true(completed);
    assert_true(out.estimated_core_temp_c >= 10.0f);
    assert_false(out.inverter_enable);
    assert_float_equal(out.target_current_amp_a, 0.0f, 0.01f);
    assert_true(out.total_energy_consumed_wh > 0.0f);
}

static void test_pulse_heat_lithium_plating_throttle(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = -25.0f,
        .pack_soc = 0.75f,
        .inverter_ready = true,
        .actual_current_rms_a = 0.0f,
        .actual_frequency_hz = 500.0f
    };
    /* 设置单体端电压逼近低温上限 (3.8 - 0.015*25 = 3.425V) */
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.40f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -25.0f;

    pulse_heat_engine_start(&engine);
    pulse_heat_control_output_t out;

    /* 推进到稳态阶段 */
    for (int i = 0; i < 50; i++) {
        pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    }

    /* 验证过电位安全裕度较小时，目标电流幅值被自适应限制 (< 150A) */
    assert_true(out.plating_safety_margin_v < 0.10f);
    assert_true(out.target_current_amp_a < 150.0f);
    assert_true(out.target_current_amp_a >= 30.0f);
}

static void test_pulse_heat_thermal_gradient_homogenize(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = -30.0f,
        .pack_soc = 0.60f,
        .inverter_ready = true,
        .actual_current_rms_a = 100.0f,
        .actual_frequency_hz = 500.0f
    };
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.20f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -30.0f;

    pulse_heat_engine_start(&engine);
    pulse_heat_control_output_t out;

    /* 人为拉大核心与表面温差 */
    engine.core_temp_est_c = -15.0f;
    engine.surf_temp_est_c = -23.0f; /* 温差 8℃ > max_thermal_gradient_c (6℃) */
    engine.current_state = PULSE_HEAT_STATE_ACTIVE_HEATING;

    pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(out.state, PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE);
    assert_float_equal(out.target_current_amp_a, 0.0f, 0.01f);
    assert_false(out.inverter_enable);
}

static void test_pulse_heat_overtemp_and_timeout_faults(void **state) {
    (void)state;
    pulse_heat_engine_t engine;
    pulse_heat_engine_init(&engine, NULL);

    pulse_heat_cell_telemetry_t telem = {
        .cell_count = 16,
        .temp_count = 4,
        .ambient_temp_c = 0.0f,
        .pack_soc = 0.50f,
        .inverter_ready = true,
        .actual_current_rms_a = 50.0f,
        .actual_frequency_hz = 500.0f
    };
    for (int i = 0; i < 16; i++) telem.cell_voltages[i] = 3.30f;
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = 48.0f; /* 超过 45℃ */

    pulse_heat_engine_start(&engine);
    pulse_heat_control_output_t out;

    pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FAULT);
    assert_int_equal(out.fault, PULSE_HEAT_FAULT_OVERTEMP);

    /* 超时故障测试 */
    pulse_heat_engine_reset(&engine);
    for (int i = 0; i < 4; i++) telem.cell_temps[i] = -10.0f;
    pulse_heat_engine_start(&engine);
    engine.total_elapsed_s = 650.0f; /* 超过 600s 超时 */

    pulse_heat_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(out.state, PULSE_HEAT_STATE_FAULT);
    assert_int_equal(out.fault, PULSE_HEAT_FAULT_TIMEOUT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pulse_heat_init_and_config),
        cmocka_unit_test(test_pulse_heat_precheck_safety_gates),
        cmocka_unit_test(test_pulse_heat_frequency_sweep_and_ramp),
        cmocka_unit_test(test_pulse_heat_full_heating_cycle_convergence),
        cmocka_unit_test(test_pulse_heat_lithium_plating_throttle),
        cmocka_unit_test(test_pulse_heat_thermal_gradient_homogenize),
        cmocka_unit_test(test_pulse_heat_overtemp_and_timeout_faults),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
