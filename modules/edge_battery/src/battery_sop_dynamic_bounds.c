/**
 * @file battery_sop_dynamic_bounds.c
 * @brief 电池峰值充放电功率 (SOP) 多约束物理边界动态预测求解器实现 (AEP-740 / Issue #270)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include "battery_sop_dynamic_bounds.h"
#include <math.h>
#include <string.h>

#define EPSILON 1e-9
#define KELVIN_OFFSET 273.15

/* ---------------- OCV 曲线与导数查找模型 ---------------- */

static double ocv_lookup_lfp(double soc) {
    if (soc <= 0.0) return 2.50;
    if (soc >= 1.0) return 3.65;
    double s = soc;
    return 2.50 + 0.65 * pow(s, 0.2) + 0.15 * s + 0.35 * (s * s * s * s);
}

static double docv_dsoc_lfp(double soc) {
    if (soc <= 0.001) soc = 0.001;
    if (soc >= 0.999) soc = 0.999;
    double s = soc;
    double d = 0.65 * 0.2 * pow(s, -0.8) + 0.15 + 0.35 * 4.0 * (s * s * s);
    return (d < 0.05) ? 0.05 : d;
}

static double ocv_lookup_nmc(double soc) {
    if (soc <= 0.0) return 2.80;
    if (soc >= 1.0) return 4.20;
    double s = soc;
    return 2.80 + 0.90 * s + 0.50 * (s * s);
}

static double docv_dsoc_nmc(double soc) {
    if (soc <= 0.001) soc = 0.001;
    if (soc >= 0.999) soc = 0.999;
    double s = soc;
    double d = 0.90 + 1.00 * s;
    return (d < 0.05) ? 0.05 : d;
}

static double ocv_lookup_sodium(double soc) {
    if (soc <= 0.0) return 1.80;
    if (soc >= 1.0) return 3.90;
    double s = soc;
    return 1.80 + 1.50 * s + 0.60 * (s * s);
}

static double docv_dsoc_sodium(double soc) {
    if (soc <= 0.001) soc = 0.001;
    if (soc >= 0.999) soc = 0.999;
    double s = soc;
    double d = 1.50 + 1.20 * s;
    return (d < 0.05) ? 0.05 : d;
}

static double compute_ocv(battery_chem_type_t chem, double soc) {
    switch (chem) {
        case BATTERY_CHEM_LFP:
            return ocv_lookup_lfp(soc);
        case BATTERY_CHEM_NMC:
            return ocv_lookup_nmc(soc);
        case BATTERY_CHEM_SODIUM_ION:
            return ocv_lookup_sodium(soc);
        default:
            return ocv_lookup_lfp(soc);
    }
}

static double compute_docv_dsoc(battery_chem_type_t chem, double soc) {
    switch (chem) {
        case BATTERY_CHEM_LFP:
            return docv_dsoc_lfp(soc);
        case BATTERY_CHEM_NMC:
            return docv_dsoc_nmc(soc);
        case BATTERY_CHEM_SODIUM_ION:
            return docv_dsoc_sodium(soc);
        default:
            return docv_dsoc_lfp(soc);
    }
}

/* ---------------- 内阻阿伦尼乌斯温度补偿 ---------------- */

static double compute_r0_temp_compensated(const battery_sop_cell_spec_t *spec, double temp_c) {
    if (!spec) return 0.002;
    if (spec->temp_coeff_r0 <= 0.0) return spec->r0_ref_ohm;

    double t_k = temp_c + KELVIN_OFFSET;
    double t_ref_k = spec->temp_ref_c + KELVIN_OFFSET;
    if (t_k <= 0.0 || t_ref_k <= 0.0) return spec->r0_ref_ohm;

    double factor = exp(spec->temp_coeff_r0 * (1.0 / t_k - 1.0 / t_ref_k));
    if (factor < 0.2) factor = 0.2;
    if (factor > 10.0) factor = 10.0;
    return spec->r0_ref_ohm * factor;
}

/* ---------------- 默认规格初始化 ---------------- */

bool battery_sop_spec_init_default(battery_sop_cell_spec_t *spec,
                                  battery_chem_type_t chem,
                                  double capacity_ah) {
    if (!spec || capacity_ah <= 0.0) {
        return false;
    }
    memset(spec, 0, sizeof(*spec));
    spec->chem_type = chem;
    spec->capacity_ah = capacity_ah;
    spec->temp_ref_c = 25.0;
    spec->coulombic_eff_charge = 0.995;
    spec->coulombic_eff_discharge = 0.998;

    switch (chem) {
        case BATTERY_CHEM_LFP:
            spec->v_min = 2.50;
            spec->v_max = 3.65;
            spec->soc_min = 0.05;
            spec->soc_max = 0.95;
            spec->max_charge_current_a = capacity_ah * 1.5;   /* 1.5C 脉冲充电 */
            spec->max_discharge_current_a = capacity_ah * 3.0;/* 3.0C 脉冲放电 */
            spec->r0_ref_ohm = 0.03 / capacity_ah;            /* ~0.3 mOhm @ 100Ah */
            spec->rp_ref_ohm = 0.04 / capacity_ah;            /* ~0.4 mOhm @ 100Ah */
            spec->cp_farad = 1500.0 * (capacity_ah / 100.0);
            spec->temp_coeff_r0 = 1200.0;
            spec->temp_cold_derate_start_c = 15.0;
            spec->temp_cold_stop_charge_c = 0.0;
            spec->temp_hot_derate_start_c = 45.0;
            spec->temp_hot_cutoff_c = 55.0;
            break;

        case BATTERY_CHEM_NMC:
            spec->v_min = 2.80;
            spec->v_max = 4.20;
            spec->soc_min = 0.05;
            spec->soc_max = 0.95;
            spec->max_charge_current_a = capacity_ah * 2.0;   /* 2.0C 脉冲充电 */
            spec->max_discharge_current_a = capacity_ah * 4.0;/* 4.0C 脉冲放电 */
            spec->r0_ref_ohm = 0.025 / capacity_ah;
            spec->rp_ref_ohm = 0.035 / capacity_ah;
            spec->cp_farad = 2000.0 * (capacity_ah / 100.0);
            spec->temp_coeff_r0 = 1100.0;
            spec->temp_cold_derate_start_c = 10.0;
            spec->temp_cold_stop_charge_c = -5.0;
            spec->temp_hot_derate_start_c = 45.0;
            spec->temp_hot_cutoff_c = 60.0;
            break;

        case BATTERY_CHEM_SODIUM_ION:
            spec->v_min = 1.80;
            spec->v_max = 3.90;
            spec->soc_min = 0.02;
            spec->soc_max = 0.98;
            spec->max_charge_current_a = capacity_ah * 3.0;   /* 钠电支持高倍率快充 */
            spec->max_discharge_current_a = capacity_ah * 5.0;
            spec->r0_ref_ohm = 0.035 / capacity_ah;
            spec->rp_ref_ohm = 0.045 / capacity_ah;
            spec->cp_farad = 1800.0 * (capacity_ah / 100.0);
            spec->temp_coeff_r0 = 800.0;                       /* 钠电低温特性优良 */
            spec->temp_cold_derate_start_c = 0.0;
            spec->temp_cold_stop_charge_c = -20.0;
            spec->temp_hot_derate_start_c = 45.0;
            spec->temp_hot_cutoff_c = 60.0;
            break;
    }

    return true;
}

/* ---------------- 单时间尺度 SOP 求解核心 ---------------- */

bool battery_sop_solve_single_horizon(const battery_sop_cell_spec_t *spec,
                                     const battery_sop_cell_state_t *state,
                                     double horizon_s,
                                     battery_sop_result_t *out_result) {
    if (!spec || !state || !out_result || horizon_s <= 0.0) {
        return false;
    }
    if (spec->capacity_ah <= 0.0 || spec->v_max <= spec->v_min) {
        return false;
    }

    memset(out_result, 0, sizeof(*out_result));
    out_result->horizon_s = horizon_s;

    double soc = state->soc;
    if (soc < 0.0) soc = 0.0;
    if (soc > 1.0) soc = 1.0;

    double temp_c = state->cell_temp_c;
    double r0 = compute_r0_temp_compensated(spec, temp_c);
    double rp = spec->rp_ref_ohm;
    double cp = (spec->cp_farad > 1.0) ? spec->cp_farad : 1000.0;
    double tau = rp * cp;
    if (tau < 0.001) tau = 0.001;

    double exp_term = exp(-horizon_s / tau);
    double ocv_cur = compute_ocv(spec->chem_type, soc);
    double docv = compute_docv_dsoc(spec->chem_type, soc);
    double q_coulombs = spec->capacity_ah * 3600.0;

    /* ================= 1. 放电极限功率求解 (Discharge SOP) ================= */

    /* 1.1 电压硬约束临界放电电流 (防过放) */
    double r_eff_dis = r0 + rp * (1.0 - exp_term) + (docv * horizon_s) / (q_coulombs * spec->coulombic_eff_discharge);
    double i_dis_volt = 0.0;
    if (r_eff_dis > EPSILON) {
        double v_headroom = ocv_cur - state->up_polarization_v * exp_term - spec->v_min;
        if (v_headroom > 0.0) {
            i_dis_volt = v_headroom / r_eff_dis;
        }
    }

    /* 1.2 SOC 边界约束临界放电电流 (防放空) */
    double i_dis_soc = 0.0;
    if (soc > spec->soc_min) {
        i_dis_soc = ((soc - spec->soc_min) * q_coulombs * spec->coulombic_eff_discharge) / horizon_s;
    }

    /* 1.3 高温过热保护降额放电电流 */
    double i_dis_thermal = spec->max_discharge_current_a;
    if (temp_c > spec->temp_hot_derate_start_c) {
        if (temp_c >= spec->temp_hot_cutoff_c) {
            i_dis_thermal = 0.0;
        } else {
            double span = spec->temp_hot_cutoff_c - spec->temp_hot_derate_start_c;
            double derate = (spec->temp_hot_cutoff_c - temp_c) / span;
            if (derate < 0.0) derate = 0.0;
            if (derate > 1.0) derate = 1.0;
            i_dis_thermal = spec->max_discharge_current_a * derate;
        }
    }

    /* 1.4 硬件最大瞬态限值 */
    double i_dis_hw = spec->max_discharge_current_a;

    /* 1.5 放电多约束最小包络与瓶颈判定 */
    double i_dis_final = i_dis_hw;
    battery_sop_limit_factor_t dis_reason = SOP_LIMIT_CURRENT_HARDWARE;

    if (i_dis_volt < i_dis_final) {
        i_dis_final = i_dis_volt;
        dis_reason = SOP_LIMIT_VOLTAGE_LOWER;
    }
    if (i_dis_soc < i_dis_final) {
        i_dis_final = i_dis_soc;
        dis_reason = SOP_LIMIT_SOC_LOWER;
    }
    if (i_dis_thermal < i_dis_final) {
        i_dis_final = i_dis_thermal;
        dis_reason = SOP_LIMIT_THERMAL_HOT;
    }
    if (i_dis_final < 0.0) {
        i_dis_final = 0.0;
    }

    /* 1.6 预测放电期末端电压与功率 */
    double delta_soc_dis = (i_dis_final * horizon_s) / (q_coulombs * spec->coulombic_eff_discharge);
    double pred_soc_dis = soc - delta_soc_dis;
    if (pred_soc_dis < 0.0) pred_soc_dis = 0.0;

    double pred_ocv_dis = compute_ocv(spec->chem_type, pred_soc_dis);
    double pred_up_dis = state->up_polarization_v * exp_term + i_dis_final * rp * (1.0 - exp_term);
    double pred_vt_dis = pred_ocv_dis - i_dis_final * r0 - pred_up_dis;
    if (pred_vt_dis < spec->v_min) pred_vt_dis = spec->v_min;

    out_result->max_discharge_current_a = i_dis_final;
    out_result->max_discharge_power_w = i_dis_final * pred_vt_dis;
    out_result->pred_terminal_v_discharge = pred_vt_dis;
    out_result->pred_soc_discharge = pred_soc_dis;
    out_result->discharge_limit_reason = (i_dis_final <= EPSILON && soc <= spec->soc_min) ? SOP_LIMIT_SOC_LOWER : dis_reason;

    /* ================= 2. 充电极限功率求解 (Charge SOP) ================= */

    /* 2.1 电压硬约束临界充电电流 (防过充析锂) */
    double r_eff_chg = r0 + rp * (1.0 - exp_term) + (docv * horizon_s * spec->coulombic_eff_charge) / q_coulombs;
    double i_chg_volt = 0.0;
    if (r_eff_chg > EPSILON) {
        double v_headroom = spec->v_max - ocv_cur - state->up_polarization_v * exp_term;
        if (v_headroom > 0.0) {
            i_chg_volt = v_headroom / r_eff_chg;
        }
    }

    /* 2.2 SOC 边界约束临界充电电流 (防充满) */
    double i_chg_soc = 0.0;
    if (soc < spec->soc_max) {
        i_chg_soc = ((spec->soc_max - soc) * q_coulombs) / (horizon_s * spec->coulombic_eff_charge);
    }

    /* 2.3 低温析锂抑制降额与高温热失控抑制降额 */
    double i_chg_thermal = spec->max_charge_current_a;
    battery_sop_limit_factor_t thermal_reason = SOP_LIMIT_NONE;

    if (temp_c < spec->temp_cold_derate_start_c) {
        if (temp_c <= spec->temp_cold_stop_charge_c) {
            i_chg_thermal = 0.0;
        } else {
            double span = spec->temp_cold_derate_start_c - spec->temp_cold_stop_charge_c;
            double derate = (temp_c - spec->temp_cold_stop_charge_c) / span;
            if (derate < 0.0) derate = 0.0;
            if (derate > 1.0) derate = 1.0;
            i_chg_thermal = spec->max_charge_current_a * derate;
        }
        thermal_reason = SOP_LIMIT_THERMAL_COLD;
    } else if (temp_c > spec->temp_hot_derate_start_c) {
        if (temp_c >= spec->temp_hot_cutoff_c) {
            i_chg_thermal = 0.0;
        } else {
            double span = spec->temp_hot_cutoff_c - spec->temp_hot_derate_start_c;
            double derate = (spec->temp_hot_cutoff_c - temp_c) / span;
            if (derate < 0.0) derate = 0.0;
            if (derate > 1.0) derate = 1.0;
            i_chg_thermal = spec->max_charge_current_a * derate;
        }
        thermal_reason = SOP_LIMIT_THERMAL_HOT;
    }

    /* 2.4 硬件最大瞬态限值 */
    double i_chg_hw = spec->max_charge_current_a;

    /* 2.5 充电多约束最小包络与瓶颈判定 */
    double i_chg_final = i_chg_hw;
    battery_sop_limit_factor_t chg_reason = SOP_LIMIT_CURRENT_HARDWARE;

    if (i_chg_volt < i_chg_final) {
        i_chg_final = i_chg_volt;
        chg_reason = SOP_LIMIT_VOLTAGE_UPPER;
    }
    if (i_chg_soc < i_chg_final) {
        i_chg_final = i_chg_soc;
        chg_reason = SOP_LIMIT_SOC_UPPER;
    }
    if (i_chg_thermal < i_chg_final) {
        i_chg_final = i_chg_thermal;
        chg_reason = thermal_reason;
    }
    if (i_chg_final < 0.0) {
        i_chg_final = 0.0;
    }

    /* 2.6 预测充电期末端电压与功率 */
    double delta_soc_chg = (i_chg_final * horizon_s * spec->coulombic_eff_charge) / q_coulombs;
    double pred_soc_chg = soc + delta_soc_chg;
    if (pred_soc_chg > 1.0) pred_soc_chg = 1.0;

    double pred_ocv_chg = compute_ocv(spec->chem_type, pred_soc_chg);
    double pred_up_chg = state->up_polarization_v * exp_term + i_chg_final * rp * (1.0 - exp_term);
    double pred_vt_chg = pred_ocv_chg + i_chg_final * r0 + pred_up_chg;
    if (pred_vt_chg > spec->v_max) pred_vt_chg = spec->v_max;

    out_result->max_charge_current_a = i_chg_final;
    out_result->max_charge_power_w = i_chg_final * pred_vt_chg;
    out_result->pred_terminal_v_charge = pred_vt_chg;
    out_result->pred_soc_charge = pred_soc_chg;
    out_result->charge_limit_reason = (i_chg_final <= EPSILON && soc >= spec->soc_max) ? SOP_LIMIT_SOC_UPPER : chg_reason;

    return true;
}

/* ---------------- 多时间尺度 SOP 矩阵求解 ---------------- */

bool battery_sop_solve_multi_horizons(const battery_sop_cell_spec_t *spec,
                                     const battery_sop_cell_state_t *state,
                                     const double *horizons_s,
                                     size_t num_horizons,
                                     battery_sop_multi_horizon_result_t *out_multi_result) {
    if (!spec || !state || !horizons_s || !out_multi_result) {
        return false;
    }
    if (num_horizons == 0 || num_horizons > BATTERY_SOP_MAX_HORIZONS) {
        return false;
    }

    memset(out_multi_result, 0, sizeof(*out_multi_result));
    out_multi_result->num_horizons = num_horizons;

    for (size_t i = 0; i < num_horizons; i++) {
        if (!battery_sop_solve_single_horizon(spec, state, horizons_s[i], &out_multi_result->horizons[i])) {
            return false;
        }
    }

    return true;
}

/* ---------------- 模组与 Pack 级拓扑缩放 ---------------- */

bool battery_sop_scale_to_pack(const battery_sop_result_t *cell_sop,
                               uint32_t series_count,
                               uint32_t parallel_count,
                               battery_sop_result_t *out_pack_sop) {
    if (!cell_sop || !out_pack_sop || series_count == 0 || parallel_count == 0) {
        return false;
    }

    *out_pack_sop = *cell_sop;

    double s = (double)series_count;
    double p = (double)parallel_count;

    out_pack_sop->max_charge_current_a = cell_sop->max_charge_current_a * p;
    out_pack_sop->max_discharge_current_a = cell_sop->max_discharge_current_a * p;
    
    out_pack_sop->pred_terminal_v_charge = cell_sop->pred_terminal_v_charge * s;
    out_pack_sop->pred_terminal_v_discharge = cell_sop->pred_terminal_v_discharge * s;
    
    out_pack_sop->max_charge_power_w = out_pack_sop->max_charge_current_a * out_pack_sop->pred_terminal_v_charge;
    out_pack_sop->max_discharge_power_w = out_pack_sop->max_discharge_current_a * out_pack_sop->pred_terminal_v_discharge;

    return true;
}

/* ---------------- 限制因子字符串映射 ---------------- */

const char *battery_sop_limit_factor_str(battery_sop_limit_factor_t factor) {
    switch (factor) {
        case SOP_LIMIT_NONE:
            return "NONE";
        case SOP_LIMIT_VOLTAGE_UPPER:
            return "VOLTAGE_UPPER_CUTOFF";
        case SOP_LIMIT_VOLTAGE_LOWER:
            return "VOLTAGE_LOWER_CUTOFF";
        case SOP_LIMIT_SOC_UPPER:
            return "SOC_UPPER_LIMIT";
        case SOP_LIMIT_SOC_LOWER:
            return "SOC_LOWER_LIMIT";
        case SOP_LIMIT_CURRENT_HARDWARE:
            return "CURRENT_HARDWARE_LIMIT";
        case SOP_LIMIT_THERMAL_COLD:
            return "THERMAL_COLD_DERATING";
        case SOP_LIMIT_THERMAL_HOT:
            return "THERMAL_HOT_DERATING";
        default:
            return "UNKNOWN";
    }
}
