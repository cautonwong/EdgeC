#ifndef BATTERY_EIS_H
#define BATTERY_EIS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_EIS_MAX_FREQ_BINS     32
#define BATTERY_EIS_DEFAULT_PERT_AMP   1.0   /**< 默认扰动电流幅值 (A) */
#define BATTERY_EIS_DEFAULT_FS_HZ      1000.0/**< 默认同步采样率 (Hz) */
#define BATTERY_EIS_DEFAULT_FMIN_HZ    0.1   /**< 默认起始低频 (Hz) */
#define BATTERY_EIS_DEFAULT_FMAX_HZ    500.0 /**< 默认截止高频 (Hz) */

/**
 * @brief PRBS 伪随机二进制序列发生器阶数
 */
typedef enum {
    BATTERY_EIS_PRBS_7  = 7,   /**< 7 阶 LFSR (序列长度 2^7 - 1 = 127) */
    BATTERY_EIS_PRBS_9  = 9,   /**< 9 阶 LFSR (序列长度 2^9 - 1 = 511) */
    BATTERY_EIS_PRBS_11 = 11   /**< 11 阶 LFSR (序列长度 2^11 - 1 = 2047) */
} battery_eis_prbs_order_t;

/**
 * @brief 单个频点的阻抗解算结果
 */
typedef struct {
    double freq_hz;          /**< 频点频率 (Hz) */
    double z_real_mohm;      /**< 阻抗实部 Z' (mΩ) */
    double z_imag_mohm;      /**< 阻抗虚部 Z'' (mΩ, 容抗为负, 感抗为正) */
    double z_mag_mohm;       /**< 阻抗模值 |Z| (mΩ) */
    double z_phase_deg;      /**< 阻抗相角 ∠Z (度, deg) */
    double coherence;        /**< 谱相干度 / 信噪比置信度 (0.0 ~ 1.0) */
} battery_eis_freq_bin_t;

/**
 * @brief Randles 等效电路模型与 Nyquist 圆弧拟合提取参数
 */
typedef struct {
    double r0_mohm;          /**< 欧姆内阻 / 溶液与接触电阻 R0 (mΩ, 高频实轴截距) */
    double r_sei_mohm;       /**< SEI 膜电阻 R_sei (mΩ) */
    double c_sei_uf;         /**< SEI 膜电容 C_sei (µF) */
    double r_ct_mohm;        /**< 电荷转移内阻 R_ct (mΩ, 动力学反应阻抗, 中频圆弧直径) */
    double c_dl_uf;          /**< 双电层电容 C_dl (µF) */
    double warburg_sigma;    /**< Warburg 固相扩散系数 σ_w (mΩ·s^(-0.5), 低频 45° 扩散斜率) */
    double f_peak_hz;        /**< Nyquist 阻抗圆弧顶点特征峰值频率 (Hz) */
    double fit_r2;           /**< 圆弧拟合优度判定系数 R^2 (0.0 ~ 1.0) */
    double soh_eis_pct;      /**< EIS 辨识健康状态评估值 (SOH, 0.0 ~ 100.0%) */
    double li_plating_risk;  /**< 低温析锂过电位与极化风险指数 (0.0 ~ 1.0) */
    bool   valid;            /**< 拟合参数是否有效且收敛 */
} battery_eis_randles_params_t;

/**
 * @brief 在线 EIS 辨识算法配置
 */
typedef struct {
    battery_eis_prbs_order_t prbs_order;   /**< PRBS 序列阶数 (7, 9, 11) */
    double pert_amplitude_a;               /**< 扰动电流幅值 (A), 推荐 0.5 ~ 2.0A */
    double sample_rate_hz;                 /**< ADC 物理同步采样率 (Hz), 推荐 1000.0 */
    uint32_t num_freq_bins;                /**< 辨识频点数量 (4 ~ BATTERY_EIS_MAX_FREQ_BINS) */
    double f_min_hz;                       /**< 最低辨识频率 (Hz), 默认 0.1 */
    double f_max_hz;                       /**< 最高辨识频率 (Hz), 默认 500.0 */
    double nominal_r0_mohm;                /**< 新电芯 25°C 标称欧姆内阻基准 (mΩ) */
    double nominal_rct_mohm;               /**< 新电芯 25°C 标称电荷转移内阻基准 (mΩ) */
} battery_eis_config_t;

#include <stdalign.h>

/**
 * @brief 在线 EIS 快速辨识器不透明聚合根类型
 */
typedef struct battery_eis_analyzer battery_eis_analyzer_t;

/**
 * @brief 调用方分配存储块 (Caller-Allocated Storage)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[8192];
} battery_eis_storage_t;

/**
 * @brief 消费者定义接口 (Consumer-Defined Ports)
 */
typedef struct {
    int (*read_sample)(void *ctx, double *current_amp, double *voltage_volt, double *cell_temp_c);
    int (*write_params)(void *ctx, const battery_eis_randles_params_t *params);
    int (*write_spectrum)(void *ctx, const battery_eis_freq_bin_t *bins, uint32_t count);
    void *ctx;
} battery_eis_ports_t;

/**
 * @brief 就地构造在线 EIS 辨识器聚合根 (0 malloc)
 * @param store 调用方分配的对齐存储
 * @param cfg 配置参数 (若为 NULL 则使用工业级默认参数)
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
battery_eis_analyzer_t *battery_eis_create_in(battery_eis_storage_t *store, const battery_eis_config_t *cfg);

/**
 * @brief 初始化在线 EIS 阻抗谱辨识器
 * @param eis 辨识器句柄
 * @param cfg 配置参数 (若为 NULL 则使用工业级默认参数)
 * @return 0 成功, 负数 错误码
 */
int battery_eis_init(battery_eis_analyzer_t *eis, const battery_eis_config_t *cfg);

/**
 * @brief 生成下一个时隙的 PRBS 宽频微扰动电流指令 ΔI (A)
 * @param eis 辨识器句柄
 * @return 扰动电流值 (例如 +1.0A 或 -1.0A, 均值为 0)
 */
double battery_eis_next_prbs_excitation(battery_eis_analyzer_t *eis);

/**
 * @brief 实时喂入一组同步采集的 (I, V) 物理采样点
 * @param eis 辨识器句柄
 * @param current_amp 实际采集端电流 (A)
 * @param voltage_volt 实际采集端电压 (V)
 * @return 0 成功, 负数 错误码
 */
int battery_eis_push_sample(battery_eis_analyzer_t *eis, double current_amp, double voltage_volt);

/**
 * @brief 查询当前采集样本量是否已满足一个完整 PRBS 周期
 * @param eis 辨识器句柄
 * @return true 样本就绪可解算, false 正在累积样本
 */
bool battery_eis_is_ready(const battery_eis_analyzer_t *eis);

/**
 * @brief 执行全频点离散傅里叶谱解调与复阻抗 Z(f) 计算
 * @param eis 辨识器句柄
 * @return 0 成功, 负数 错误码
 */
int battery_eis_solve_spectrum(battery_eis_analyzer_t *eis);

/**
 * @brief 执行 Randles 等效电路 Nyquist 圆弧非线性拟合与 SOH / 析锂风险解算
 * @param eis 辨识器句柄
 * @param cell_temp_c 当前电芯物理温度 (°C)
 * @param dc_current_amp 当前直流平均电流 (A)
 * @return 0 成功, 负数 错误码
 */
int battery_eis_fit_nyquist(battery_eis_analyzer_t *eis, double cell_temp_c, double dc_current_amp);

/**
 * @brief 流式一步处理接口 (推送样本 + 自动判定周期就绪 + 自动解算阻抗谱与拟合)
 * @param eis 辨识器句柄
 * @param current_amp 当前物理电流 (A)
 * @param voltage_volt 当前物理电压 (V)
 * @param cell_temp_c 当前物理温度 (°C)
 * @param out_ready 若本步完成了一次完整辨识则置为 true, 否则 false
 * @return 0 成功, 负数 错误码
 */
int battery_eis_step(battery_eis_analyzer_t *eis,
                     double current_amp,
                     double voltage_volt,
                     double cell_temp_c,
                     bool *out_ready);

/**
 * @brief 获取拟合后的 Randles 物理参数及健康指标
 * @param eis 辨识器句柄
 * @return 参数指针, 若未完成解算返回 NULL
 */
const battery_eis_randles_params_t *battery_eis_get_params(const battery_eis_analyzer_t *eis);

/**
 * @brief 获取全部离散频点的复阻抗谱列表
 * @param eis 辨识器句柄
 * @param out_bins 输出频点数组缓冲区 (长度至少为 num_freq_bins)
 * @param out_count 输出有效频点数
 * @return 0 成功, 负数 错误码
 */
int battery_eis_get_spectrum(const battery_eis_analyzer_t *eis,
                             battery_eis_freq_bin_t *out_bins,
                             uint32_t *out_count);

/**
 * @brief 重置辨识器状态与累加器
 * @param eis 辨识器句柄
 */
void battery_eis_reset(battery_eis_analyzer_t *eis);

/**
 * @brief 基于消费者 Ports 抽象执行单步 EIS 辨识调度 (Consumer-Defined Ports)
 * @param eis 辨识器句柄
 * @param ports 消费者接口表
 * @param out_ready 若本步完成了一次完整辨识则置为 true, 否则 false
 * @return 0 成功, 负数 错误码
 */
int battery_eis_step_ports(battery_eis_analyzer_t *eis, const battery_eis_ports_t *ports, bool *out_ready);

/**
 * @brief 探针接口: 获取配置
 */
const battery_eis_config_t *battery_eis_get_config(const battery_eis_analyzer_t *eis);

/**
 * @brief 探针接口: 获取 PRBS 周期长度
 */
uint32_t battery_eis_get_prbs_len(const battery_eis_analyzer_t *eis);

/**
 * @brief 探针接口: 获取离散频点只读数组
 */
const battery_eis_freq_bin_t *battery_eis_get_bins(const battery_eis_analyzer_t *eis);

/**
 * @brief 探针接口: 设定测试阻抗谱数据
 */
void battery_eis_set_test_spectrum(battery_eis_analyzer_t *eis,
                                   const battery_eis_freq_bin_t *bins,
                                   uint32_t count,
                                   uint32_t samples);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_EIS_H */
