/**
 * @file battery_thermal_mesh.c
 * @brief 电池单体热阻抗矩阵非均匀热场三维重建模型实现 (AEP-739)
 */

#include "battery_thermal_mesh.h"
#include <math.h>
#include <string.h>

#define GAS_CONSTANT_R 8.314462f
#define KELVIN_OFFSET 273.15f
#define REF_TEMP_K 298.15f /* 25.0 ℃ */

static const thermal_mesh_config_t DEFAULT_CONFIG = {
    .cell_mass_kg = 5.4f,               /* 280Ah LFP 标称质量 */
    .specific_heat_j_kg_k = 1100.0f,    /* LFP 典型比热容 */
    .k_in_plane_w_m_k = 28.0f,          /* 平行极片面内高导热 */
    .k_cross_plane_w_m_k = 1.1f,        /* 垂直极片叠层低导热 */
    .r_tab_pos_mohm = 0.08f,            /* 正极铝极耳寄生阻抗 */
    .r_tab_neg_mohm = 0.05f,            /* 负极铜极耳寄生阻抗 */
    .h_ambient_conv_w_k = 0.4f,         /* 自然对流换热 */
    .h_liquid_plate_w_k = 12.0f,        /* 底部液冷板换热 */
    .d_ocv_d_t_v_k = -0.0002f,          /* 熵热效应系数 */
    .max_temp_alarm_c = 55.0f,          /* 热点报警阈值 */
    .max_gradient_alarm_c = 8.0f        /* 空间温差梯度报警阈值 */
};

int thermal_mesh_engine_init(thermal_mesh_engine_t *engine, const thermal_mesh_config_t *cfg) {
    if (!engine) {
        return -1;
    }
    memset(engine, 0, sizeof(*engine));
    if (cfg) {
        engine->cfg = *cfg;
    } else {
        engine->cfg = DEFAULT_CONFIG;
    }

    /* 1. 根据电芯总热容划分各空间节点热容 (单位: J/K) */
    float c_total = engine->cfg.cell_mass_kg * engine->cfg.specific_heat_j_kg_k;
    engine->heat_capacities[THERMAL_NODE_CORE_CENTER]   = c_total * 0.40f;
    engine->heat_capacities[THERMAL_NODE_TAB_POSITIVE]  = c_total * 0.05f;
    engine->heat_capacities[THERMAL_NODE_TAB_NEGATIVE]  = c_total * 0.05f;
    engine->heat_capacities[THERMAL_NODE_TOP_SURFACE]   = c_total * 0.08f;
    engine->heat_capacities[THERMAL_NODE_BOTTOM_COOLING]= c_total * 0.12f;
    engine->heat_capacities[THERMAL_NODE_SIDE_LEFT]     = c_total * 0.075f;
    engine->heat_capacities[THERMAL_NODE_SIDE_RIGHT]    = c_total * 0.075f;
    engine->heat_capacities[THERMAL_NODE_SURFACE_FRONT] = c_total * 0.075f;
    engine->heat_capacities[THERMAL_NODE_SURFACE_BACK]  = c_total * 0.075f;

    /* 2. 构建三维非对称热导矩阵 G_ij (单位: W/K) */
    /* 面内方向与核心的高导热耦合 (x, z 轴) */
    float g_core_tab_pos = 6.0f * (engine->cfg.k_in_plane_w_m_k / 28.0f);
    float g_core_tab_neg = 6.0f * (engine->cfg.k_in_plane_w_m_k / 28.0f);
    float g_core_top     = 4.5f * (engine->cfg.k_in_plane_w_m_k / 28.0f);
    float g_core_bottom  = 8.0f * (engine->cfg.k_in_plane_w_m_k / 28.0f);
    float g_core_side_l  = 3.5f * (engine->cfg.k_in_plane_w_m_k / 28.0f);
    float g_core_side_r  = 3.5f * (engine->cfg.k_in_plane_w_m_k / 28.0f);

    /* 垂直极片跨面方向的低导热耦合 (y 轴) */
    float g_core_front   = 1.2f * (engine->cfg.k_cross_plane_w_m_k / 1.1f);
    float g_core_back    = 1.2f * (engine->cfg.k_cross_plane_w_m_k / 1.1f);

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_TAB_POSITIVE] = g_core_tab_pos;
    engine->conductance_matrix[THERMAL_NODE_TAB_POSITIVE][THERMAL_NODE_CORE_CENTER] = g_core_tab_pos;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_TAB_NEGATIVE] = g_core_tab_neg;
    engine->conductance_matrix[THERMAL_NODE_TAB_NEGATIVE][THERMAL_NODE_CORE_CENTER] = g_core_tab_neg;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_TOP_SURFACE] = g_core_top;
    engine->conductance_matrix[THERMAL_NODE_TOP_SURFACE][THERMAL_NODE_CORE_CENTER] = g_core_top;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_BOTTOM_COOLING] = g_core_bottom;
    engine->conductance_matrix[THERMAL_NODE_BOTTOM_COOLING][THERMAL_NODE_CORE_CENTER] = g_core_bottom;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_SIDE_LEFT] = g_core_side_l;
    engine->conductance_matrix[THERMAL_NODE_SIDE_LEFT][THERMAL_NODE_CORE_CENTER] = g_core_side_l;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_SIDE_RIGHT] = g_core_side_r;
    engine->conductance_matrix[THERMAL_NODE_SIDE_RIGHT][THERMAL_NODE_CORE_CENTER] = g_core_side_r;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_SURFACE_FRONT] = g_core_front;
    engine->conductance_matrix[THERMAL_NODE_SURFACE_FRONT][THERMAL_NODE_CORE_CENTER] = g_core_front;

    engine->conductance_matrix[THERMAL_NODE_CORE_CENTER][THERMAL_NODE_SURFACE_BACK] = g_core_back;
    engine->conductance_matrix[THERMAL_NODE_SURFACE_BACK][THERMAL_NODE_CORE_CENTER] = g_core_back;

    /* 极耳至顶盖直接热阻 */
    engine->conductance_matrix[THERMAL_NODE_TAB_POSITIVE][THERMAL_NODE_TOP_SURFACE] = 2.0f;
    engine->conductance_matrix[THERMAL_NODE_TOP_SURFACE][THERMAL_NODE_TAB_POSITIVE] = 2.0f;
    engine->conductance_matrix[THERMAL_NODE_TAB_NEGATIVE][THERMAL_NODE_TOP_SURFACE] = 2.0f;
    engine->conductance_matrix[THERMAL_NODE_TOP_SURFACE][THERMAL_NODE_TAB_NEGATIVE] = 2.0f;

    /* 侧面与表面向底部冷板传导热阻 */
    engine->conductance_matrix[THERMAL_NODE_SIDE_LEFT][THERMAL_NODE_BOTTOM_COOLING] = 1.5f;
    engine->conductance_matrix[THERMAL_NODE_BOTTOM_COOLING][THERMAL_NODE_SIDE_LEFT] = 1.5f;
    engine->conductance_matrix[THERMAL_NODE_SIDE_RIGHT][THERMAL_NODE_BOTTOM_COOLING] = 1.5f;
    engine->conductance_matrix[THERMAL_NODE_BOTTOM_COOLING][THERMAL_NODE_SIDE_RIGHT] = 1.5f;

    engine->conductance_matrix[THERMAL_NODE_SURFACE_FRONT][THERMAL_NODE_BOTTOM_COOLING] = 1.0f;
    engine->conductance_matrix[THERMAL_NODE_BOTTOM_COOLING][THERMAL_NODE_SURFACE_FRONT] = 1.0f;
    engine->conductance_matrix[THERMAL_NODE_SURFACE_BACK][THERMAL_NODE_BOTTOM_COOLING] = 1.0f;
    engine->conductance_matrix[THERMAL_NODE_BOTTOM_COOLING][THERMAL_NODE_SURFACE_BACK] = 1.0f;

    /* 3. 外部边界散热导热率 */
    float h_amb = engine->cfg.h_ambient_conv_w_k;
    engine->ambient_conductance[THERMAL_NODE_TOP_SURFACE]   = h_amb * 0.35f;
    engine->ambient_conductance[THERMAL_NODE_TAB_POSITIVE]  = h_amb * 0.05f;
    engine->ambient_conductance[THERMAL_NODE_TAB_NEGATIVE]  = h_amb * 0.05f;
    engine->ambient_conductance[THERMAL_NODE_SIDE_LEFT]     = h_amb * 0.15f;
    engine->ambient_conductance[THERMAL_NODE_SIDE_RIGHT]    = h_amb * 0.15f;
    engine->ambient_conductance[THERMAL_NODE_SURFACE_FRONT] = h_amb * 0.125f;
    engine->ambient_conductance[THERMAL_NODE_SURFACE_BACK]  = h_amb * 0.125f;

    /* 底部液冷板换热导热率 */
    engine->cooling_conductance[THERMAL_NODE_BOTTOM_COOLING] = engine->cfg.h_liquid_plate_w_k;

    /* 默认初始温度 25.0 ℃ */
    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        engine->temperatures[i] = 25.0f;
    }

    engine->initialized = true;
    return 0;
}

int thermal_mesh_engine_reset(thermal_mesh_engine_t *engine, float init_temp_c) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        engine->temperatures[i] = init_temp_c;
    }
    return 0;
}

int thermal_mesh_engine_step(thermal_mesh_engine_t *engine,
                             const thermal_mesh_telemetry_t *telem,
                             float dt_s,
                             thermal_mesh_output_t *out) {
    if (!engine || !engine->initialized || !telem || !out) {
        return -1;
    }
    if (dt_s <= 0.0f) {
        dt_s = 0.01f;
    }
    if (dt_s > 1.0f) {
        dt_s = 1.0f;
    }

    const thermal_mesh_config_t *cfg = &engine->cfg;
    float current_a = telem->load_current_a;
    float abs_curr = fabsf(current_a);

    /* 1. 计算发热源功率向量 Q_gen */
    /* 核心欧姆内阻与 Arrhenius 温度修正 */
    float t_core_k = engine->temperatures[THERMAL_NODE_CORE_CENTER] + KELVIN_OFFSET;
    if (t_core_k < 200.0f) t_core_k = 200.0f;
    float r_ohm_base = 0.00025f; /* 280Ah LFP 标称内阻 0.25 mΩ */
    float r_ohm = r_ohm_base * expf((25000.0f / GAS_CONSTANT_R) * ((1.0f / t_core_k) - (1.0f / REF_TEMP_K)));

    float q_joule = (current_a * current_a) * r_ohm;

    /* 电化学极化发热功率 (过电位 = |端电压 - 开路电压| - 欧姆压降) */
    float v_diff = fabsf(telem->cell_terminal_voltage_v - telem->open_circuit_voltage_v);
    float eta_pol = v_diff - (abs_curr * r_ohm);
    if (eta_pol < 0.0f) eta_pol = 0.0f;
    float q_pol = abs_curr * eta_pol;

    /* 可逆熵热发热功率: Q_rev = -I * T * (dOCV/dT) */
    float q_rev = -current_a * t_core_k * cfg->d_ocv_d_t_v_k;

    /* 极耳寄生发热功率: Q_tab = I^2 * R_tab */
    float q_tab_pos = (current_a * current_a) * (cfg->r_tab_pos_mohm * 0.001f);
    float q_tab_neg = (current_a * current_a) * (cfg->r_tab_neg_mohm * 0.001f);

    float q_gen_nodes[THERMAL_MESH_NUM_NODES] = {0};
    q_gen_nodes[THERMAL_NODE_CORE_CENTER]  = q_joule + q_pol + q_rev;
    q_gen_nodes[THERMAL_NODE_TAB_POSITIVE] = q_tab_pos;
    q_gen_nodes[THERMAL_NODE_TAB_NEGATIVE] = q_tab_neg;

    /* 2. 状态方程离散积分: C_i * dT_i/dt = Q_gen_i + sum(G_ij*(T_j - T_i)) - G_ext*(T_i - T_ext) */
    float d_temp[THERMAL_MESH_NUM_NODES] = {0};
    float total_cooling_heat_w = 0.0f;
    float total_ambient_heat_w = 0.0f;

    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        float q_conduction = 0.0f;
        for (int j = 0; j < THERMAL_MESH_NUM_NODES; j++) {
            if (i != j && engine->conductance_matrix[i][j] > 0.0f) {
                q_conduction += engine->conductance_matrix[i][j] *
                                (engine->temperatures[j] - engine->temperatures[i]);
            }
        }

        /* 环境对流散失 */
        float q_amb = 0.0f;
        if (engine->ambient_conductance[i] > 0.0f) {
            q_amb = engine->ambient_conductance[i] *
                    (telem->ambient_temp_c - engine->temperatures[i]);
            total_ambient_heat_w += -q_amb;
        }

        /* 底板液冷散失 */
        float q_cool = 0.0f;
        if (engine->cooling_conductance[i] > 0.0f) {
            q_cool = engine->cooling_conductance[i] *
                     (telem->coolant_temp_c - engine->temperatures[i]);
            total_cooling_heat_w += -q_cool;
        }

        float q_net = q_gen_nodes[i] + q_conduction + q_amb + q_cool;
        d_temp[i] = (q_net / engine->heat_capacities[i]) * dt_s;
    }

    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        engine->temperatures[i] += d_temp[i];
    }

    /* 3. 结合物理表面传感器实测温度进行状态观测器卡尔曼/自适应补偿 */
    if (telem->sensor_count > 0) {
        float obs_gain = 0.12f * (dt_s / 0.1f);
        if (obs_gain > 0.35f) obs_gain = 0.35f;

        for (uint8_t s = 0; s < telem->sensor_count && s < THERMAL_MESH_MAX_SENSORS; s++) {
            if (telem->bindings[s].is_valid) {
                thermal_node_id_t node = telem->bindings[s].mapped_node;
                if (node < THERMAL_MESH_NUM_NODES) {
                    float residual = telem->sensor_temps[s] - engine->temperatures[node];
                    engine->temperatures[node] += obs_gain * residual;

                    /* 将表面残差沿强导热路径适度传导反馈至内部核心节点 */
                    if (node != THERMAL_NODE_CORE_CENTER) {
                        float coupling_weight = engine->conductance_matrix[node][THERMAL_NODE_CORE_CENTER] / 10.0f;
                        if (coupling_weight > 0.5f) coupling_weight = 0.5f;
                        engine->temperatures[THERMAL_NODE_CORE_CENTER] += (obs_gain * coupling_weight) * residual;
                    }
                }
            }
        }
    }

    /* 4. 统计热场特征指标 */
    float t_max = -1000.0f;
    float t_min = 1000.0f;
    thermal_node_id_t max_node = THERMAL_NODE_CORE_CENTER;

    for (int i = 0; i < THERMAL_MESH_NUM_NODES; i++) {
        float t = engine->temperatures[i];
        out->node_temperatures[i] = t;
        if (t > t_max) {
            t_max = t;
            max_node = (thermal_node_id_t)i;
        }
        if (t < t_min) {
            t_min = t;
        }
    }

    float max_grad = t_max - t_min;
    float tab_grad = fabsf(engine->temperatures[THERMAL_NODE_TAB_POSITIVE] -
                           engine->temperatures[THERMAL_NODE_CORE_CENTER]);
    float tab_neg_grad = fabsf(engine->temperatures[THERMAL_NODE_TAB_NEGATIVE] -
                               engine->temperatures[THERMAL_NODE_CORE_CENTER]);
    if (tab_neg_grad > tab_grad) {
        tab_grad = tab_neg_grad;
    }

    /* 5. 组装输出结果 */
    out->core_hotspot_temp_c = t_max;
    out->hotspot_node = max_node;
    out->min_temp_c = t_min;
    out->max_thermal_gradient_c = max_grad;
    out->tab_gradient_c = tab_grad;
    out->total_heat_gen_power_w = q_joule + q_pol + q_rev + q_tab_pos + q_tab_neg;
    out->joule_heat_power_w = q_joule;
    out->polarization_heat_power_w = q_pol;
    out->reversible_entropic_heat_w = q_rev;
    out->cooling_heat_flux_w = total_cooling_heat_w;
    out->ambient_loss_heat_w = total_ambient_heat_w;
    out->hotspot_alarm = (t_max >= cfg->max_temp_alarm_c);
    out->gradient_alarm = (max_grad >= cfg->max_gradient_alarm_c);

    return 0;
}

const char *thermal_mesh_node_to_str(thermal_node_id_t node) {
    switch (node) {
    case THERMAL_NODE_CORE_CENTER: return "CORE_CENTER";
    case THERMAL_NODE_TAB_POSITIVE: return "TAB_POSITIVE";
    case THERMAL_NODE_TAB_NEGATIVE: return "TAB_NEGATIVE";
    case THERMAL_NODE_TOP_SURFACE: return "TOP_SURFACE";
    case THERMAL_NODE_BOTTOM_COOLING: return "BOTTOM_COOLING";
    case THERMAL_NODE_SIDE_LEFT: return "SIDE_LEFT";
    case THERMAL_NODE_SIDE_RIGHT: return "SIDE_RIGHT";
    case THERMAL_NODE_SURFACE_FRONT: return "SURFACE_FRONT";
    case THERMAL_NODE_SURFACE_BACK: return "SURFACE_BACK";
    default: return "UNKNOWN";
    }
}
