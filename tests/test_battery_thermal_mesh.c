/**
 * @file test_battery_thermal_mesh.c
 * @brief 电池单体热阻抗矩阵非均匀热场三维重建模型单元测试 (AEP-739)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "battery_thermal_mesh.h"

static void test_thermal_mesh_init_and_reset(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;

    int ret = thermal_mesh_engine_init(&engine, NULL);
    assert_int_equal(ret, 0);
    assert_true(engine.initialized);

    /* 验证热网络节点名称字符串转换 */
    assert_string_equal(thermal_mesh_node_to_str(THERMAL_NODE_CORE_CENTER), "CORE_CENTER");
    assert_string_equal(thermal_mesh_node_to_str(THERMAL_NODE_TAB_POSITIVE), "TAB_POSITIVE");
    assert_string_equal(thermal_mesh_node_to_str(THERMAL_NODE_BOTTOM_COOLING), "BOTTOM_COOLING");
    assert_string_equal(thermal_mesh_node_to_str(THERMAL_NODE_SURFACE_FRONT), "SURFACE_FRONT");

    /* 验证导热矩阵对称性 G_ij == G_ji */
    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        for (int j = 0; j < THERMAL_MESH_NUM_NODES; j++) {
            assert_float_equal(engine.conductance_matrix[i][j], engine.conductance_matrix[j][i],
                               0.0001f);
        }
    }

    /* 验证重置温度 */
    ret = thermal_mesh_engine_reset(&engine, 30.0f);
    assert_int_equal(ret, 0);
    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        assert_float_equal(engine.temperatures[i], 30.0f, 0.001f);
    }
}

static void test_thermal_mesh_steady_state_no_load(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    thermal_mesh_telemetry_t telem = {.load_current_a = 0.0f,
                                      .cell_terminal_voltage_v = 3.30f,
                                      .open_circuit_voltage_v = 3.30f,
                                      .cell_soc = 0.50f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 0};

    thermal_mesh_output_t out;
    int ret = thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(ret, 0);

    /* 零电流稳态无发热，各节点保持 25℃ */
    assert_float_equal(out.total_heat_gen_power_w, 0.0f, 0.001f);
    assert_float_equal(out.core_hotspot_temp_c, 25.0f, 0.01f);
    assert_float_equal(out.max_thermal_gradient_c, 0.0f, 0.01f);
    assert_false(out.hotspot_alarm);
    assert_false(out.gradient_alarm);
}

static void test_thermal_mesh_high_rate_discharge_heating(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 模拟 280A (1C) 恒流放电工况 */
    thermal_mesh_telemetry_t telem = {.load_current_a = 280.0f,
                                      .cell_terminal_voltage_v = 3.15f,
                                      .open_circuit_voltage_v = 3.28f, /* 极化过电位 0.13V */
                                      .cell_soc = 0.80f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 0};

    thermal_mesh_output_t out;

    /* 仿真连续放电 300 秒 (5 分钟) */
    for (int i = 0; i < 3000; i++) {
        thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    }

    /* 验证产热量 */
    assert_true(out.total_heat_gen_power_w > 20.0f);
    assert_true(out.joule_heat_power_w > 10.0f);
    assert_true(out.polarization_heat_power_w > 5.0f);

    /* 核心热点温升应明显高于表面大面节点 (非对称热阻效应: k_in_plane vs k_cross_plane) */
    assert_true(out.core_hotspot_temp_c > 25.0f);
    assert_true(out.node_temperatures[THERMAL_NODE_CORE_CENTER] >
                out.node_temperatures[THERMAL_NODE_SURFACE_FRONT]);
    assert_true(out.max_thermal_gradient_c > 0.5f);
}

static void test_thermal_mesh_tab_temperature_rise(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 模拟高电流脉冲 (400A) 验证极耳局部收敛发热 */
    thermal_mesh_telemetry_t telem = {.load_current_a = 400.0f,
                                      .cell_terminal_voltage_v = 3.10f,
                                      .open_circuit_voltage_v = 3.25f,
                                      .cell_soc = 0.70f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 0};

    thermal_mesh_output_t out;
    for (int i = 0; i < 600; i++) { /* 60 秒大电流脉冲 */
        thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    }

    /* 验证正负极耳温升与梯度 */
    assert_true(out.node_temperatures[THERMAL_NODE_TAB_POSITIVE] > 25.5f);
    assert_true(out.node_temperatures[THERMAL_NODE_TAB_NEGATIVE] > 25.5f);
    assert_true(out.tab_gradient_c >= 0.0f);
}

static void test_thermal_mesh_bottom_liquid_cooling(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 设置底板进水温度为 15.0 ℃, 环境 25.0 ℃ */
    thermal_mesh_telemetry_t telem = {.load_current_a = 150.0f,
                                      .cell_terminal_voltage_v = 3.20f,
                                      .open_circuit_voltage_v = 3.26f,
                                      .cell_soc = 0.60f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 15.0f,
                                      .sensor_count = 0};

    thermal_mesh_output_t out;
    for (int i = 0; i < 1000; i++) {
        thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    }

    /* 底部液冷接触面温度应低于内部核心温度 */
    assert_true(out.node_temperatures[THERMAL_NODE_BOTTOM_COOLING] <
                out.node_temperatures[THERMAL_NODE_CORE_CENTER]);
    /* 导出至液冷板的散热热流应为正 */
    assert_true(out.cooling_heat_flux_w > 0.0f);
}

static void test_thermal_mesh_surface_sensor_fusion(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 接入 1 个前大面 NTC 传感器 (测得 35.0 ℃，而模型此时初始为 25.0 ℃) */
    thermal_mesh_telemetry_t telem = {.load_current_a = 0.0f,
                                      .cell_terminal_voltage_v = 3.30f,
                                      .open_circuit_voltage_v = 3.30f,
                                      .cell_soc = 0.50f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 1,
                                      .sensor_temps = {35.0f},
                                      .bindings = {{.mapped_node = THERMAL_NODE_SURFACE_FRONT,
                                                    .sensor_accuracy_c = 0.2f,
                                                    .is_valid = true}}};

    thermal_mesh_output_t out;

    /* 步进若干周期观察状态观测器收敛 */
    for (int i = 0; i < 50; i++) {
        thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    }

    /* 表面节点温度应被观测器拉升至接近实测 35.0 ℃ */
    assert_true(out.node_temperatures[THERMAL_NODE_SURFACE_FRONT] > 30.0f);
    assert_true(out.node_temperatures[THERMAL_NODE_SURFACE_FRONT] <= 35.5f);
}

static void test_thermal_mesh_hotspot_and_gradient_alarms(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    thermal_mesh_telemetry_t telem = {.load_current_a = 0.0f,
                                      .cell_terminal_voltage_v = 3.30f,
                                      .open_circuit_voltage_v = 3.30f,
                                      .cell_soc = 0.50f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 0};

    /* 人为设置核心超温 (> 55.0 ℃) */
    engine.temperatures[THERMAL_NODE_CORE_CENTER] = 58.0f;
    engine.temperatures[THERMAL_NODE_BOTTOM_COOLING] = 45.0f; /* 梯度 13℃ > 8℃ */

    thermal_mesh_output_t out;
    thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);

    assert_true(out.hotspot_alarm);
    assert_true(out.gradient_alarm);
    assert_int_equal(out.hotspot_node, THERMAL_NODE_CORE_CENTER);
    assert_float_equal(out.core_hotspot_temp_c, 58.0f, 0.5f);
}

static void test_thermal_mesh_sensor_nan_rejection(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 注入包含 NAN、Inf 及越界温度的传感器脏数据 */
    thermal_mesh_telemetry_t telem = {
        .load_current_a = 50.0f,
        .cell_terminal_voltage_v = 3.25f,
        .open_circuit_voltage_v = 3.28f,
        .cell_soc = 0.60f,
        .ambient_temp_c = 25.0f,
        .coolant_temp_c = 25.0f,
        .sensor_count = 3,
        .sensor_temps = {(float)NAN, (float)INFINITY, -99.0f},
        .bindings = {{.mapped_node = THERMAL_NODE_SURFACE_FRONT, .is_valid = true},
                     {.mapped_node = THERMAL_NODE_TAB_POSITIVE, .is_valid = true},
                     {.mapped_node = THERMAL_NODE_TAB_NEGATIVE, .is_valid = true}}};

    thermal_mesh_output_t out;
    for (int i = 0; i < 20; i++) {
        int ret = thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
        assert_int_equal(ret, 0);
    }

    /* 验证热网络未被 NAN/Inf 毒化，所有节点温度均保持有限合规值 */
    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        assert_true(isfinite(out.node_temperatures[i]));
        assert_true(out.node_temperatures[i] >= 20.0f && out.node_temperatures[i] <= 100.0f);
    }
}

static void test_thermal_mesh_invalid_binding_bounds(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    /* 传入有符号负数及越界枚举作为绑定节点 */
    thermal_mesh_telemetry_t telem = {
        .load_current_a = 0.0f,
        .cell_terminal_voltage_v = 3.30f,
        .open_circuit_voltage_v = 3.30f,
        .cell_soc = 0.50f,
        .ambient_temp_c = 25.0f,
        .coolant_temp_c = 25.0f,
        .sensor_count = 2,
        .sensor_temps = {35.0f, 40.0f},
        .bindings = {{.mapped_node = (thermal_node_id_t)(-1), .is_valid = true},
                     {.mapped_node = (thermal_node_id_t)99, .is_valid = true}}};

    thermal_mesh_output_t out;
    int ret = thermal_mesh_engine_step(&engine, &telem, 0.1f, &out);
    assert_int_equal(ret, 0);

    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        assert_true(isfinite(out.node_temperatures[i]));
    }
}

static void test_thermal_mesh_invalid_cfg_rejection(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;

    thermal_mesh_config_t bad_cfg_mass = {.cell_mass_kg = 0.0f, .specific_heat_j_kg_k = 1100.0f};
    assert_int_equal(thermal_mesh_engine_init(&engine, &bad_cfg_mass), -1);

    thermal_mesh_config_t bad_cfg_cp = {.cell_mass_kg = 5.4f, .specific_heat_j_kg_k = -10.0f};
    assert_int_equal(thermal_mesh_engine_init(&engine, &bad_cfg_cp), -1);

    thermal_mesh_config_t bad_cfg_k = {
        .cell_mass_kg = 5.4f, .specific_heat_j_kg_k = 1100.0f, .k_in_plane_w_m_k = -1.0f};
    assert_int_equal(thermal_mesh_engine_init(&engine, &bad_cfg_k), -1);
}

static void test_thermal_mesh_nan_dt_rejection(void **state) {
    (void)state;
    thermal_mesh_engine_t engine;
    thermal_mesh_engine_init(&engine, NULL);

    thermal_mesh_telemetry_t telem = {.load_current_a = 100.0f,
                                      .cell_terminal_voltage_v = 3.20f,
                                      .open_circuit_voltage_v = 3.28f,
                                      .cell_soc = 0.70f,
                                      .ambient_temp_c = 25.0f,
                                      .coolant_temp_c = 25.0f,
                                      .sensor_count = 0};

    thermal_mesh_output_t out;
    /* 传入 NAN 与 Inf 步长，验证被安全兜底，无发散 */
    int ret1 = thermal_mesh_engine_step(&engine, &telem, (float)NAN, &out);
    assert_int_equal(ret1, 0);
    assert_true(isfinite(out.core_hotspot_temp_c));

    int ret2 = thermal_mesh_engine_step(&engine, &telem, (float)INFINITY, &out);
    assert_int_equal(ret2, 0);
    assert_true(isfinite(out.core_hotspot_temp_c));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_thermal_mesh_init_and_reset),
        cmocka_unit_test(test_thermal_mesh_steady_state_no_load),
        cmocka_unit_test(test_thermal_mesh_high_rate_discharge_heating),
        cmocka_unit_test(test_thermal_mesh_tab_temperature_rise),
        cmocka_unit_test(test_thermal_mesh_bottom_liquid_cooling),
        cmocka_unit_test(test_thermal_mesh_surface_sensor_fusion),
        cmocka_unit_test(test_thermal_mesh_hotspot_and_gradient_alarms),
        cmocka_unit_test(test_thermal_mesh_sensor_nan_rejection),
        cmocka_unit_test(test_thermal_mesh_invalid_binding_bounds),
        cmocka_unit_test(test_thermal_mesh_invalid_cfg_rejection),
        cmocka_unit_test(test_thermal_mesh_nan_dt_rejection),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
