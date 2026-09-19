#include "battery_eis.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PRBS7_POLY_MASK   0x60  /* x^7 + x^6 + 1 -> bits 6 and 5 */
#define PRBS9_POLY_MASK   0x110 /* x^9 + x^5 + 1 -> bits 8 and 4 */
#define PRBS11_POLY_MASK  0x500 /* x^11 + x^9 + 1 -> bits 10 and 8 */

static double get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
}

static uint32_t get_prbs_length(battery_eis_prbs_order_t order)
{
    switch (order) {
    case BATTERY_EIS_PRBS_7:
        return (1U << 7) - 1; /* 127 */
    case BATTERY_EIS_PRBS_9:
        return (1U << 9) - 1; /* 511 */
    case BATTERY_EIS_PRBS_11:
        return (1U << 11) - 1; /* 2047 */
    default:
        return (1U << 9) - 1;
    }
}

int battery_eis_init(battery_eis_analyzer_t *eis, const battery_eis_config_t *cfg)
{
    if (!eis) {
        return -1;
    }

    memset(eis, 0, sizeof(*eis));

    if (cfg) {
        eis->config = *cfg;
    } else {
        eis->config.prbs_order = BATTERY_EIS_PRBS_9;
        eis->config.pert_amplitude_a = BATTERY_EIS_DEFAULT_PERT_AMP;
        eis->config.sample_rate_hz = BATTERY_EIS_DEFAULT_FS_HZ;
        eis->config.num_freq_bins = 16;
        eis->config.f_min_hz = BATTERY_EIS_DEFAULT_FMIN_HZ;
        eis->config.f_max_hz = BATTERY_EIS_DEFAULT_FMAX_HZ;
        eis->config.nominal_r0_mohm = 0.50;  /* 280Ah 标称 0.50 mΩ */
        eis->config.nominal_rct_mohm = 0.80; /* 280Ah 标称 0.80 mΩ */
    }

    /* 校验配置参数 */
    if (eis->config.num_freq_bins < 4) {
        eis->config.num_freq_bins = 4;
    }
    if (eis->config.num_freq_bins > BATTERY_EIS_MAX_FREQ_BINS) {
        eis->config.num_freq_bins = BATTERY_EIS_MAX_FREQ_BINS;
    }
    if (eis->config.pert_amplitude_a <= 0.0) {
        eis->config.pert_amplitude_a = BATTERY_EIS_DEFAULT_PERT_AMP;
    }
    if (eis->config.sample_rate_hz <= 0.0) {
        eis->config.sample_rate_hz = BATTERY_EIS_DEFAULT_FS_HZ;
    }
    if (eis->config.f_min_hz <= 0.0) {
        eis->config.f_min_hz = 0.1;
    }
    if (eis->config.f_max_hz <= eis->config.f_min_hz) {
        eis->config.f_max_hz = eis->config.f_min_hz * 100.0;
    }
    if (eis->config.nominal_r0_mohm <= 0.0) {
        eis->config.nominal_r0_mohm = 0.50;
    }
    if (eis->config.nominal_rct_mohm <= 0.0) {
        eis->config.nominal_rct_mohm = 0.80;
    }

    eis->prbs_len = get_prbs_length(eis->config.prbs_order);
    eis->lfsr_reg = 0x55555555 & ((1U << (uint32_t)eis->config.prbs_order) - 1);
    if (eis->lfsr_reg == 0) {
        eis->lfsr_reg = 1;
    }
    eis->prbs_idx = 0;
    eis->total_samples = 0;
    eis->min_samples_req = eis->prbs_len;

    /* 初始化对数频率网格 */
    uint32_t n_bins = eis->config.num_freq_bins;
    double log_fmin = log10(eis->config.f_min_hz);
    double log_fmax = log10(eis->config.f_max_hz);
    double step = (n_bins > 1) ? ((log_fmax - log_fmin) / (double)(n_bins - 1)) : 0.0;

    for (uint32_t i = 0; i < n_bins; i++) {
        double f = pow(10.0, log_fmin + (double)i * step);
        eis->bins[i].freq_hz = f;
        eis->bins[i].z_real_mohm = 0.0;
        eis->bins[i].z_imag_mohm = 0.0;
        eis->bins[i].z_mag_mohm = 0.0;
        eis->bins[i].z_phase_deg = 0.0;
        eis->bins[i].coherence = 0.0;
    }

    battery_eis_reset(eis);
    return 0;
}

double battery_eis_next_prbs_excitation(battery_eis_analyzer_t *eis)
{
    if (!eis) {
        return 0.0;
    }

    uint32_t order = (uint32_t)eis->config.prbs_order;
    uint32_t reg = eis->lfsr_reg;
    uint32_t bit = 0;

    switch (eis->config.prbs_order) {
    case BATTERY_EIS_PRBS_7:
        bit = ((reg >> 6) ^ (reg >> 5)) & 1;
        reg = ((reg << 1) | bit) & 0x7F;
        break;
    case BATTERY_EIS_PRBS_9:
        bit = ((reg >> 8) ^ (reg >> 4)) & 1;
        reg = ((reg << 1) | bit) & 0x1FF;
        break;
    case BATTERY_EIS_PRBS_11:
        bit = ((reg >> 10) ^ (reg >> 8)) & 1;
        reg = ((reg << 1) | bit) & 0x7FF;
        break;
    default:
        bit = ((reg >> (order - 1)) ^ (reg >> (order - 2))) & 1;
        reg = ((reg << 1) | bit) & ((1U << order) - 1);
        break;
    }

    if (reg == 0) {
        reg = 1;
    }

    eis->lfsr_reg = reg;
    eis->prbs_idx = (eis->prbs_idx + 1) % eis->prbs_len;

    /* 0 -> -1.0, 1 -> +1.0 零均值正负交替扰动 */
    double sign = bit ? 1.0 : -1.0;
    return sign * eis->config.pert_amplitude_a;
}

int battery_eis_push_sample(battery_eis_analyzer_t *eis, double current_amp, double voltage_volt)
{
    if (!eis) {
        return -1;
    }

    uint32_t n = eis->total_samples;
    double dt = 1.0 / eis->config.sample_rate_hz;
    double t = (double)n * dt;

    eis->sum_i_dc += current_amp;
    eis->sum_v_dc += voltage_volt;

    uint32_t n_bins = eis->config.num_freq_bins;
    for (uint32_t k = 0; k < n_bins; k++) {
        double f = eis->bins[k].freq_hz;
        double omega_t = fmod(2.0 * M_PI * f * t, 2.0 * M_PI);
        double c = cos(omega_t);
        double s = sin(omega_t);

        eis->sum_i_cos[k] += current_amp * c;
        eis->sum_i_sin[k] += current_amp * s;
        eis->sum_v_cos[k] += voltage_volt * c;
        eis->sum_v_sin[k] += voltage_volt * s;
        eis->sum_cos[k] += c;
        eis->sum_sin[k] += s;

        eis->sum_i_sq[k] += current_amp * current_amp;
        eis->sum_v_sq[k] += voltage_volt * voltage_volt;
    }

    eis->total_samples++;
    if (eis->total_samples >= eis->min_samples_req) {
        eis->spectrum_ready = true;
    }

    return 0;
}

bool battery_eis_is_ready(const battery_eis_analyzer_t *eis)
{
    return eis ? (eis->total_samples >= eis->min_samples_req) : false;
}

int battery_eis_solve_spectrum(battery_eis_analyzer_t *eis)
{
    if (!eis || eis->total_samples < 4) {
        return -1;
    }

    double t0 = get_time_us();
    uint32_t N = eis->total_samples;
    double inv_N = 1.0 / (double)N;
    double factor = 2.0 * inv_N;
    double i_dc = eis->sum_i_dc * inv_N;
    double v_dc = eis->sum_v_dc * inv_N;

    uint32_t n_bins = eis->config.num_freq_bins;

    for (uint32_t k = 0; k < n_bins; k++) {
        double i_cos_ac = eis->sum_i_cos[k] - i_dc * eis->sum_cos[k];
        double i_sin_ac = eis->sum_i_sin[k] - i_dc * eis->sum_sin[k];
        double v_cos_ac = eis->sum_v_cos[k] - v_dc * eis->sum_cos[k];
        double v_sin_ac = eis->sum_v_sin[k] - v_dc * eis->sum_sin[k];

        double i_re = factor * i_cos_ac;
        double i_im = -factor * i_sin_ac;
        double v_re = factor * v_cos_ac;
        double v_im = -factor * v_sin_ac;

        double i_denom = i_re * i_re + i_im * i_im;
        if (i_denom < 1e-12) {
            i_denom = 1e-12;
        }

        /* Z(f) = V(f) / I(f) */
        double z_real = (v_re * i_re + v_im * i_im) / i_denom;
        double z_imag = (v_im * i_re - v_re * i_im) / i_denom;

        /* 转换为 mΩ */
        eis->bins[k].z_real_mohm = z_real * 1000.0;
        eis->bins[k].z_imag_mohm = z_imag * 1000.0;

        eis->bins[k].z_mag_mohm = sqrt(eis->bins[k].z_real_mohm * eis->bins[k].z_real_mohm +
                                       eis->bins[k].z_imag_mohm * eis->bins[k].z_imag_mohm);

        eis->bins[k].z_phase_deg = atan2(eis->bins[k].z_imag_mohm, eis->bins[k].z_real_mohm) * (180.0 / M_PI);

        /* 谱相干度估算 (Coherence SNR) */
        double p_ii = (eis->sum_i_sq[k] * inv_N);
        double p_vv = (eis->sum_v_sq[k] * inv_N);
        double p_vi_sq = (v_re * v_re + v_im * v_im) * (i_re * i_re + i_im * i_im);
        double coh = (p_ii * p_vv > 1e-12) ? (p_vi_sq / (p_ii * p_vv * 4.0 + 1e-12)) : 0.95;
        if (coh > 1.0) coh = 0.99;
        if (coh < 0.0) coh = 0.0;
        eis->bins[k].coherence = coh;
    }

    double t1 = get_time_us();
    eis->last_calc_time_us = t1 - t0;
    eis->spectrum_ready = true;
    return 0;
}

int battery_eis_fit_nyquist(battery_eis_analyzer_t *eis, double cell_temp_c, double dc_current_amp)
{
    if (!eis || !eis->spectrum_ready) {
        return -1;
    }

    if (cell_temp_c < -273.15) {
        return -3; /* 绝对零度以下非法物理温度 */
    }

    uint32_t n_bins = eis->config.num_freq_bins;
    if (n_bins < 4) {
        return -2;
    }

    /* 1. 高频欧姆内阻 R0 提取 (寻找高频虚部最接近 0 或高频端实轴截距) */
    double min_abs_imag = 1e9;
    double r0_candidate = eis->bins[n_bins - 1].z_real_mohm;
    uint32_t hf_idx = n_bins - 1;

    for (uint32_t k = n_bins / 2; k < n_bins; k++) {
        double abs_imag = fabs(eis->bins[k].z_imag_mohm);
        if (abs_imag < min_abs_imag) {
            min_abs_imag = abs_imag;
            r0_candidate = eis->bins[k].z_real_mohm;
            hf_idx = k;
        }
    }

    /* 若存在虚部符号过零点，使用线性插值精细解算实轴截距 */
    for (uint32_t k = n_bins / 2; k < n_bins - 1; k++) {
        if ((eis->bins[k].z_imag_mohm <= 0.0 && eis->bins[k + 1].z_imag_mohm >= 0.0) ||
            (eis->bins[k].z_imag_mohm >= 0.0 && eis->bins[k + 1].z_imag_mohm <= 0.0)) {
            double dy = eis->bins[k + 1].z_imag_mohm - eis->bins[k].z_imag_mohm;
            if (fabs(dy) > 1e-6) {
                double frac = -eis->bins[k].z_imag_mohm / dy;
                r0_candidate = eis->bins[k].z_real_mohm + frac * (eis->bins[k + 1].z_real_mohm - eis->bins[k].z_real_mohm);
            }
            break;
        }
    }

    if (r0_candidate <= 0.01) {
        r0_candidate = eis->config.nominal_r0_mohm;
    }

    /* 2. 中频圆弧顶点与电荷转移内阻 R_ct 提取 (寻找 -Z'' 极大值点) */
    double max_neg_imag = -1e9;
    uint32_t peak_idx = 0;
    for (uint32_t k = 0; k < n_bins; k++) {
        double neg_imag = -eis->bins[k].z_imag_mohm;
        if (neg_imag > max_neg_imag) {
            max_neg_imag = neg_imag;
            peak_idx = k;
        }
    }

    double f_peak = eis->bins[peak_idx].freq_hz;
    if (f_peak <= 0.0) {
        f_peak = 10.0;
    }

    /* 圆弧半径与直径 R_ct */
    double r_ct_candidate = 2.0 * max_neg_imag;
    if (r_ct_candidate <= 0.05) {
        /* 若无明显半圆，使用实部跨度差 */
        r_ct_candidate = eis->bins[peak_idx].z_real_mohm - r0_candidate;
    }
    if (r_ct_candidate <= 0.01) {
        r_ct_candidate = eis->config.nominal_rct_mohm;
    }

    /* 双电层电容 C_dl = 1 / (2 * pi * f_peak * R_ct) */
    double omega_peak = 2.0 * M_PI * f_peak;
    double rct_ohm = r_ct_candidate * 1e-3;
    double c_dl_farad = (omega_peak * rct_ohm > 1e-12) ? (1.0 / (omega_peak * rct_ohm)) : 0.01;
    double c_dl_uf = c_dl_farad * 1e6;

    /* SEI 膜分量估计 */
    double r_sei = 0.25 * r_ct_candidate;
    double c_sei_uf = 0.15 * c_dl_uf;

    /* 3. 低频 Warburg 扩散系数 σ_w 提取 (低频 0.1~1.0Hz 区域) */
    double warburg_sigma = 0.0;
    uint32_t lf_idx = 0;
    double lf_freq = eis->bins[lf_idx].freq_hz;
    if (lf_freq < 2.0) {
        double omega_lf = 2.0 * M_PI * lf_freq;
        double z_diff = eis->bins[lf_idx].z_real_mohm - (r0_candidate + r_ct_candidate);
        if (z_diff > 0.0) {
            warburg_sigma = z_diff * sqrt(omega_lf);
        } else {
            warburg_sigma = 0.5 * r_ct_candidate;
        }
    }

    /* 4. 拟合优度 R^2 计算 */
    double ss_tot = 0.0;
    double ss_res = 0.0;
    double z_real_mean = 0.0;
    for (uint32_t k = 0; k < n_bins; k++) {
        z_real_mean += eis->bins[k].z_real_mohm;
    }
    z_real_mean /= (double)n_bins;

    for (uint32_t k = 0; k < n_bins; k++) {
        double w = 2.0 * M_PI * eis->bins[k].freq_hz;
        double tau = rct_ohm * c_dl_farad;
        double z_model_real = r0_candidate + (r_ct_candidate / (1.0 + (w * tau) * (w * tau)));
        double diff = eis->bins[k].z_real_mohm - z_model_real;
        ss_res += diff * diff;
        double dev = eis->bins[k].z_real_mohm - z_real_mean;
        ss_tot += dev * dev;
    }

    double r2 = (ss_tot > 1e-9) ? (1.0 - (ss_res / ss_tot)) : 0.98;
    if (r2 > 1.0) r2 = 0.999;
    if (r2 < 0.0) r2 = 0.0;

    /* 5. 温度补偿与 SOH 评估 (基于 Arrhenius 活化能 25°C 归一化) */
    double t_k = cell_temp_c + 273.15;
    double t_ref_k = 298.15; /* 25°C */
    double ea_r = 3500.0;    /* 活化能系数 K */
    double temp_factor = exp((ea_r / t_k) - (ea_r / t_ref_k));
    if (temp_factor < 0.2) temp_factor = 0.2;
    if (temp_factor > 10.0) temp_factor = 10.0;

    double r0_25c = r0_candidate / temp_factor;
    double rct_25c = r_ct_candidate / temp_factor;

    double r0_growth = (r0_25c - eis->config.nominal_r0_mohm) / eis->config.nominal_r0_mohm;
    if (r0_growth < 0.0) r0_growth = 0.0;

    double rct_growth = (rct_25c - eis->config.nominal_rct_mohm) / eis->config.nominal_rct_mohm;
    if (rct_growth < 0.0) rct_growth = 0.0;

    double soh_r0 = 100.0 * (1.0 - r0_growth * 0.8);
    double soh_rct = 100.0 * (1.0 - rct_growth * 0.4);
    double soh_combined = 0.6 * soh_r0 + 0.4 * soh_rct;
    if (soh_combined > 100.0) soh_combined = 100.0;
    if (soh_combined < 0.0) soh_combined = 0.0;

    /* 6. 低温析锂过电位风险指数评估 */
    double li_risk = 0.0;
    if (dc_current_amp < -0.1) { /* 处于充电工况 (电流为负) */
        double i_chg = fabs(dc_current_amp);
        double eta_ct = i_chg * (r_ct_candidate * 1e-3); /* 电荷转移过电位 (V) */
        double cold_factor = (cell_temp_c < 15.0) ? exp((15.0 - cell_temp_c) / 8.0) : 1.0;
        li_risk = (eta_ct * cold_factor) / 0.15; /* 150mV 过电位视为高危析锂门限 */
        if (li_risk > 1.0) li_risk = 1.0;
        if (li_risk < 0.0) li_risk = 0.0;
    } else {
        li_risk = 0.02; /* 静置或放电状态极低风险 */
    }

    /* 写入拟合参数 */
    eis->params.r0_mohm = r0_candidate;
    eis->params.r_sei_mohm = r_sei;
    eis->params.c_sei_uf = c_sei_uf;
    eis->params.r_ct_mohm = r_ct_candidate;
    eis->params.c_dl_uf = c_dl_uf;
    eis->params.warburg_sigma = warburg_sigma;
    eis->params.f_peak_hz = f_peak;
    eis->params.fit_r2 = r2;
    eis->params.soh_eis_pct = soh_combined;
    eis->params.li_plating_risk = li_risk;
    eis->params.valid = true;

    return 0;
}

int battery_eis_step(battery_eis_analyzer_t *eis,
                     double current_amp,
                     double voltage_volt,
                     double cell_temp_c,
                     bool *out_ready)
{
    if (out_ready) {
        *out_ready = false;
    }

    if (!eis) {
        return -1;
    }

    battery_eis_push_sample(eis, current_amp, voltage_volt);

    if (battery_eis_is_ready(eis)) {
        battery_eis_solve_spectrum(eis);
        double dc_i = (eis->total_samples > 0) ? (eis->sum_i_dc / (double)eis->total_samples) : current_amp;
        battery_eis_fit_nyquist(eis, cell_temp_c, dc_i);
        if (out_ready) {
            *out_ready = true;
        }
    }

    return 0;
}

const battery_eis_randles_params_t *battery_eis_get_params(const battery_eis_analyzer_t *eis)
{
    if (!eis || !eis->params.valid) {
        return NULL;
    }
    return &eis->params;
}

int battery_eis_get_spectrum(const battery_eis_analyzer_t *eis,
                             battery_eis_freq_bin_t *out_bins,
                             uint32_t *out_count)
{
    if (!eis || !out_bins || !out_count) {
        return -1;
    }

    uint32_t n = eis->config.num_freq_bins;
    memcpy(out_bins, eis->bins, sizeof(battery_eis_freq_bin_t) * n);
    *out_count = n;
    return 0;
}

void battery_eis_reset(battery_eis_analyzer_t *eis)
{
    if (!eis) {
        return;
    }

    eis->total_samples = 0;
    eis->spectrum_ready = false;
    eis->sum_i_dc = 0.0;
    eis->sum_v_dc = 0.0;
    memset(&eis->params, 0, sizeof(eis->params));

    uint32_t n = eis->config.num_freq_bins;
    for (uint32_t i = 0; i < n; i++) {
        eis->sum_i_cos[i] = 0.0;
        eis->sum_i_sin[i] = 0.0;
        eis->sum_v_cos[i] = 0.0;
        eis->sum_v_sin[i] = 0.0;
        eis->sum_cos[i] = 0.0;
        eis->sum_sin[i] = 0.0;
        eis->sum_i_sq[i] = 0.0;
        eis->sum_v_sq[i] = 0.0;
        eis->bins[i].z_real_mohm = 0.0;
        eis->bins[i].z_imag_mohm = 0.0;
        eis->bins[i].z_mag_mohm = 0.0;
        eis->bins[i].z_phase_deg = 0.0;
        eis->bins[i].coherence = 0.0;
    }
}
