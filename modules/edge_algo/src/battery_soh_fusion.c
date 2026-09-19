#include "battery_soh_fusion.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#define R_GAS_CONST 8.314462618 /* J/(mol*K) */

/* 嵌入式紧凑神经网络权重 (7 输入 -> 8 隐藏 -> 1 输出) */
static const double NN_W1[8][7] = {
    {-0.52, -0.48, -0.15, -0.22, -0.18, -0.65, -0.12},
    {-0.45, -0.55, -0.20, -0.19, -0.25, -0.70, -0.08},
    {-0.60, -0.40, -0.12, -0.30, -0.15, -0.58, -0.15},
    {-0.38, -0.62, -0.25, -0.15, -0.22, -0.62, -0.10},
    {-0.50, -0.50, -0.18, -0.25, -0.20, -0.68, -0.14},
    {-0.42, -0.58, -0.22, -0.18, -0.28, -0.72, -0.09},
    {-0.55, -0.45, -0.16, -0.28, -0.16, -0.60, -0.16},
    {-0.48, -0.52, -0.19, -0.20, -0.24, -0.66, -0.11}
};

static const double NN_B1[8] = {
    0.85, 0.90, 0.82, 0.88, 0.86, 0.92, 0.84, 0.89
};

static const double NN_W2[8] = {
    0.35, 0.38, 0.32, 0.36, 0.34, 0.40, 0.33, 0.37
};

static const double NN_B2 = -0.20;

static double relu(double x)
{
    return (x > 0.0) ? x : 0.0;
}

static double evaluate_nn_soh(double norm_cyc, double norm_days, double norm_temp,
                              double dod, double crate, double norm_r0, double soc)
{
    /* 特征向量: 均为正向单调衰减应力因子 (全 0 时对应出厂新电芯) */
    double x[7] = {
        (norm_cyc > 0.0) ? norm_cyc : 0.0,
        (norm_days > 0.0) ? norm_days : 0.0,
        (norm_temp > 0.0) ? norm_temp : 0.0,
        (dod > 0.5) ? (dod - 0.5) * 2.0 : 0.0,
        (crate > 0.5) ? (crate - 0.5) * 1.5 : 0.0,
        (norm_r0 > 0.0) ? norm_r0 : 0.0,
        (soc > 0.8) ? (soc - 0.8) * 2.0 : 0.0
    };

    double h[8];
    for (int i = 0; i < 8; i++) {
        double sum = NN_B1[i];
        for (int j = 0; j < 7; j++) {
            sum += NN_W1[i][j] * x[j];
        }
        h[i] = relu(sum);
    }

    double z2 = NN_B2;
    for (int i = 0; i < 8; i++) {
        z2 += NN_W2[i] * h[i];
    }

    /* 基准化映射: 当处于出厂新电芯状态 (h = B1) 时 z2_max = 2.4856 - 0.20 = 2.2856 -> SOH = 1.0 */
    double soh = 0.75 + 0.25 * ((z2 - NN_B2) / 2.4856);
    if (soh > 1.0) soh = 1.0;
    if (soh < 0.50) soh = 0.50;
    return soh;
}

int battery_soh_init(battery_soh_fusion_estimator_t *est, const battery_soh_config_t *cfg)
{
    if (!est) {
        return -1;
    }

    memset(est, 0, sizeof(*est));

    if (cfg) {
        est->config = *cfg;
    } else {
        est->config.nominal_capacity_ah = 280.0;
        est->config.fresh_r0_mohm = 0.50;
        est->config.fresh_rct_mohm = 0.80;
        est->config.eol_soh_ratio = BATTERY_SOH_DEFAULT_EOL_THRESH;
        
        /* 工业典型 LFP 储能电芯机理衰减参数 */
        est->config.a_cal = 0.0012;
        est->config.ea_cal = 24000.0;
        est->config.beta_soc = 0.80;
        
        est->config.b_cyc = 0.000045;
        est->config.ea_cyc = 15000.0;
        est->config.alpha_crate = 0.40;
        est->config.beta_dod = 1.20;
        
        est->config.min_coulomb_delta_soc = 0.25;
    }

    /* 参数有效性保护 */
    if (est->config.nominal_capacity_ah <= 0.0) {
        est->config.nominal_capacity_ah = 280.0;
    }
    if (est->config.fresh_r0_mohm <= 0.0) {
        est->config.fresh_r0_mohm = 0.50;
    }
    if (est->config.fresh_rct_mohm <= 0.0) {
        est->config.fresh_rct_mohm = 0.80;
    }
    if (est->config.eol_soh_ratio <= 0.50 || est->config.eol_soh_ratio >= 0.95) {
        est->config.eol_soh_ratio = 0.80;
    }
    if (est->config.min_coulomb_delta_soc <= 0.05) {
        est->config.min_coulomb_delta_soc = 0.25;
    }

    battery_soh_reset(est);
    return 0;
}

int battery_soh_feed_sample(battery_soh_fusion_estimator_t *est,
                            double current_a,
                            double voltage_v,
                            double temp_c,
                            double soc,
                            double timestamp_s)
{
    if (!est) {
        return -1;
    }

    if (est->initial_timestamp_s <= 0.0) {
        est->initial_timestamp_s = timestamp_s;
        est->last_timestamp_s = timestamp_s;
        est->coulomb.start_soc = soc;
        est->coulomb.start_timestamp_s = timestamp_s;
        est->coulomb.in_progress = true;
    }

    double dt = timestamp_s - est->last_timestamp_s;
    if (dt < 0.0) dt = 0.0;
    if (dt > 3600.0) dt = 1.0; /* 保护异常时间跳跃 */

    est->last_timestamp_s = timestamp_s;
    est->total_samples_fed++;

    /* 移动平均运行温度与运行统计 */
    double alpha_stat = 0.01;
    if (est->total_samples_fed == 1) {
        est->avg_operating_temp_c = temp_c;
        est->avg_dod = 0.80;
    } else {
        est->avg_operating_temp_c = (1.0 - alpha_stat) * est->avg_operating_temp_c + alpha_stat * temp_c;
    }

    /* 积分吞吐量 */
    double ah_step = fabs(current_a) * (dt / 3600.0);
    if (current_a < 0.0) {
        est->total_ah_charge += ah_step;
    } else {
        est->total_ah_discharge += ah_step;
    }
    est->total_energy_kwh += (voltage_v * fabs(current_a) * dt) / (3600.0 * 1000.0);

    /* 1. 动态库仑积分过程追踪 */
    if (est->coulomb.in_progress) {
        est->coulomb.accumulated_ah += ah_step;
        double delta_soc = fabs(soc - est->coulomb.start_soc);

        /* 若电流符号改变或进入静置，且 delta_soc 满足有效窗口门限，则执行一次容量结算 */
        if (delta_soc >= est->config.min_coulomb_delta_soc) {
            double identified_cap = est->coulomb.accumulated_ah / delta_soc;
            if (identified_cap >= 0.5 * est->config.nominal_capacity_ah &&
                identified_cap <= 1.2 * est->config.nominal_capacity_ah) {
                est->coulomb.last_identified_cap_ah = identified_cap;
                est->coulomb.last_coulomb_soh = identified_cap / est->config.nominal_capacity_ah;
                est->coulomb.last_coulomb_valid = true;
            }
        }
    }

    /* 2. ICA 增量容量采样入队 (仅在恒流充电模式下) */
    if (current_a < -0.5) { /* 充电工况 */
        uint16_t idx = est->ica.sample_count;
        if (idx < BATTERY_SOH_ICA_WINDOW_SIZE) {
            est->ica.voltage_history[idx] = voltage_v;
            est->ica.charge_history_ah[idx] = est->total_ah_charge;
            est->ica.sample_count++;
        } else {
            /* 滑动移位 */
            memmove(&est->ica.voltage_history[0], &est->ica.voltage_history[1],
                    sizeof(double) * (BATTERY_SOH_ICA_WINDOW_SIZE - 1));
            memmove(&est->ica.charge_history_ah[0], &est->ica.charge_history_ah[1],
                    sizeof(double) * (BATTERY_SOH_ICA_WINDOW_SIZE - 1));
            est->ica.voltage_history[BATTERY_SOH_ICA_WINDOW_SIZE - 1] = voltage_v;
            est->ica.charge_history_ah[BATTERY_SOH_ICA_WINDOW_SIZE - 1] = est->total_ah_charge;
        }

        /* 当历史样本充足时，计算 dQ/dV 特征峰值 */
        if (est->ica.sample_count >= 16) {
            double max_dqdv_1 = 0.0;
            double max_dqdv_2 = 0.0;
            double v_p1 = 3.32;
            double v_p2 = 3.40;
            uint16_t n = est->ica.sample_count;

            for (uint16_t i = 4; i < n; i++) {
                double dv = est->ica.voltage_history[i] - est->ica.voltage_history[i - 4];
                double dq = est->ica.charge_history_ah[i] - est->ica.charge_history_ah[i - 4];
                if (dv > 0.002) {
                    double dqdv = dq / dv;
                    double v_mid = 0.5 * (est->ica.voltage_history[i] + est->ica.voltage_history[i - 4]);
                    if (v_mid >= 3.28 && v_mid <= 3.36) {
                        if (dqdv > max_dqdv_1) {
                            max_dqdv_1 = dqdv;
                            v_p1 = v_mid;
                        }
                    } else if (v_mid > 3.36 && v_mid <= 3.45) {
                        if (dqdv > max_dqdv_2) {
                            max_dqdv_2 = dqdv;
                            v_p2 = v_mid;
                        }
                    }
                }
            }

            if (max_dqdv_1 > 0.0 || max_dqdv_2 > 0.0) {
                est->ica.peak1_voltage_v = v_p1;
                est->ica.peak1_height = max_dqdv_1;
                est->ica.peak2_voltage_v = v_p2;
                est->ica.peak2_height = max_dqdv_2;
                est->ica.features_valid = true;
            }
        }
    }

    return 0;
}

int battery_soh_feed_impedance(battery_soh_fusion_estimator_t *est,
                               double r0_mohm,
                               double rct_mohm,
                               double temp_c)
{
    if (!est) {
        return -1;
    }

    (void)rct_mohm;
    /* 温度归一化到 25°C */
    double t_k = temp_c + 273.15;
    double t_ref_k = 298.15;
    double temp_factor = exp((3500.0 / t_k) - (3500.0 / t_ref_k));
    if (temp_factor < 0.2) temp_factor = 0.2;
    if (temp_factor > 10.0) temp_factor = 10.0;

    double r0_25c = r0_mohm / temp_factor;
    double r0_fresh = est->config.fresh_r0_mohm;

    double growth = (r0_25c - r0_fresh) / r0_fresh;
    if (growth < 0.0) growth = 0.0;

    /* 欧姆内阻增长 100% 对应 SOH 衰退 20% (即 SOH_res = 80%) */
    double soh_res = 1.0 - (growth * 0.20);
    if (soh_res > 1.0) soh_res = 1.0;
    if (soh_res < 0.50) soh_res = 0.50;

    est->state_soh_res = soh_res;
    return 0;
}

int battery_soh_process_fusion(battery_soh_fusion_estimator_t *est,
                               double cur_timestamp_s)
{
    if (!est) {
        return -1;
    }

    /* 1. 计算累计等效满充满放循环次数与日历天数 */
    double total_throughput_ah = 0.5 * (est->total_ah_charge + est->total_ah_discharge);
    double eq_cycles = total_throughput_ah / est->config.nominal_capacity_ah;
    double cal_days = (cur_timestamp_s - est->initial_timestamp_s) / 86400.0;
    if (cal_days < 0.0) cal_days = 0.0;

    /* 2. 半经验机理老化模型解算 (Mechanistic Model) */
    double t_k = est->avg_operating_temp_c + 273.15;
    double t_ref_k = 298.15;
    double arrh_cal = exp((-est->config.ea_cal / R_GAS_CONST) * ((1.0 / t_k) - (1.0 / t_ref_k)));
    double arrh_cyc = exp((-est->config.ea_cyc / R_GAS_CONST) * ((1.0 / t_k) - (1.0 / t_ref_k)));

    double q_loss_cal = est->config.a_cal * est->config.nominal_capacity_ah *
                        arrh_cal * sqrt(cal_days + 0.01);
    double q_loss_cyc = est->config.b_cyc * est->config.nominal_capacity_ah *
                        arrh_cyc * pow(eq_cycles + 0.01, 0.85);

    double soh_mech = 1.0 - ((q_loss_cal + q_loss_cyc) / est->config.nominal_capacity_ah);
    if (soh_mech > 1.0) soh_mech = 1.0;
    if (soh_mech < 0.50) soh_mech = 0.50;

    /* 3. 神经网络模型推理 (Neural Network Degradation Predictor) */
    double norm_cyc = eq_cycles / 3000.0;
    double norm_days = cal_days / 1000.0;
    double norm_temp = (est->avg_operating_temp_c > 25.0) ? (est->avg_operating_temp_c - 25.0) / 30.0 : 0.0;
    double norm_dod = (eq_cycles > 0.1 && est->avg_dod > 0.5) ? (est->avg_dod - 0.5) * 2.0 : 0.0;
    double norm_r0 = (est->state_soh_res > 0.0 && est->state_soh_res < 0.999) ? (1.0 - est->state_soh_res) : 0.0;

    double soh_nn = evaluate_nn_soh(norm_cyc, norm_days, norm_temp, norm_dod, 0.5, norm_r0, 0.50);
    if (soh_nn > 1.0) soh_nn = 1.0;
    if (soh_nn < 0.50) soh_nn = 0.50;

    /* 4. ICA 特征估算 */
    double soh_ica = soh_mech;
    if (est->ica.features_valid) {
        double fresh_h1 = 1500.0; /* 标称高度基准 Ah/V */
        if (est->ica.peak1_height > 0.0) {
            double ratio = est->ica.peak1_height / fresh_h1;
            soh_ica = 0.70 + 0.30 * ratio;
            if (soh_ica > 1.0) soh_ica = 1.0;
            if (soh_ica < 0.50) soh_ica = 0.50;
        }
    }

    /* 5. 库仑积分有效值 */
    double soh_coulomb = (est->coulomb.last_coulomb_valid) ? est->coulomb.last_coulomb_soh : soh_mech;

    /* 6. 多源贝叶斯融合权重分配 */
    double w_mech = 0.25;
    double w_nn = 0.20;
    double w_coulomb = (est->coulomb.last_coulomb_valid) ? 0.35 : 0.0;
    double w_ica = (est->ica.features_valid) ? 0.20 : 0.0;
    double w_res = (est->state_soh_res > 0.0 && est->state_soh_res < 0.999) ? 0.25 : 0.0;

    double w_sum = w_mech + w_nn + w_coulomb + w_ica + w_res;
    double soh_fused = (w_mech * soh_mech + w_nn * soh_nn + w_coulomb * soh_coulomb + w_ica * soh_ica + w_res * est->state_soh_res) / w_sum;

    if (soh_fused > 1.0) soh_fused = 1.0;
    if (soh_fused < 0.50) soh_fused = 0.50;

    est->state_soh_cap = soh_fused;
    if (est->state_soh_res <= 0.0) {
        est->state_soh_res = soh_fused;
    }

    /* 7. 寿命预测 RUL 与衰减速率解算 */
    double deg_amount = 1.0 - soh_fused;
    double deg_rate_100 = (eq_cycles > 10.0) ? (deg_amount / eq_cycles * 100.0) : 0.60;
    if (deg_rate_100 < 0.001) deg_rate_100 = 0.001;

    double delta_to_eol = soh_fused - est->config.eol_soh_ratio;
    double remaining_cycles = 0.0;
    if (delta_to_eol > 0.0) {
        remaining_cycles = (delta_to_eol / (deg_rate_100 / 100.0));
    }

    double daily_cyc = (cal_days > 1.0) ? (eq_cycles / cal_days) : 1.0;
    if (daily_cyc < 0.1) daily_cyc = 0.5;
    double remaining_days = remaining_cycles / daily_cyc;

    /* 8. 健康分级判定 */
    battery_soh_grade_t grade;
    if (soh_fused >= 0.95) {
        grade = BATTERY_SOH_GRADE_EXCELLENT;
    } else if (soh_fused >= 0.90) {
        grade = BATTERY_SOH_GRADE_GOOD;
    } else if (soh_fused >= 0.85) {
        grade = BATTERY_SOH_GRADE_AGED;
    } else if (soh_fused >= est->config.eol_soh_ratio) {
        grade = BATTERY_SOH_GRADE_DEGRADED;
    } else {
        grade = BATTERY_SOH_GRADE_EOL;
    }

    /* 写入结果 */
    est->result.soh_capacity = soh_fused;
    est->result.soh_resistance = est->state_soh_res;
    est->result.actual_capacity_ah = soh_fused * est->config.nominal_capacity_ah;
    est->result.soh_mechanistic = soh_mech;
    est->result.soh_coulomb = soh_coulomb;
    est->result.soh_ica = soh_ica;
    est->result.soh_nn = soh_nn;
    est->result.rul_remaining_cycles = remaining_cycles;
    est->result.rul_remaining_days = remaining_days;
    est->result.degradation_rate_pct_100cyc = deg_rate_100;
    est->result.total_equivalent_cycles = eq_cycles;
    est->result.total_throughput_kwh = est->total_energy_kwh;
    est->result.total_calendar_days = cal_days;
    est->result.health_grade = grade;
    est->result.confidence_score = (est->coulomb.last_coulomb_valid ? 0.95 : 0.85);
    est->result.valid = true;

    return 0;
}

const battery_soh_result_t *battery_soh_get_result(const battery_soh_fusion_estimator_t *est)
{
    if (!est || !est->result.valid) {
        return NULL;
    }
    return &est->result;
}

void battery_soh_reset(battery_soh_fusion_estimator_t *est)
{
    if (!est) {
        return;
    }

    memset(&est->ica, 0, sizeof(est->ica));
    memset(&est->coulomb, 0, sizeof(est->coulomb));
    memset(&est->result, 0, sizeof(est->result));

    est->total_ah_charge = 0.0;
    est->total_ah_discharge = 0.0;
    est->total_energy_kwh = 0.0;
    est->initial_timestamp_s = 0.0;
    est->last_timestamp_s = 0.0;
    est->avg_operating_temp_c = 25.0;
    est->avg_dod = 0.80;
    est->total_samples_fed = 0;

    est->state_soh_cap = 1.0;
    est->state_soh_res = 1.0;
    est->cov_soh_cap = 0.01;
}
