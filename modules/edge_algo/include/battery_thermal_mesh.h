/**
 * @file battery_thermal_mesh.h
 * @brief 电池单体热阻抗矩阵非均匀热场三维重建模型 (AEP-739)
 * @details 基于各向异性导热机理与三维降阶热网络 (Reduced-Order Model)，
 *          实时重建电芯内部不可测核心热点、极耳温升与三维非对称热场分布，消除热盲区。
 */

#ifndef BATTERY_THERMAL_MESH_H
#define BATTERY_THERMAL_MESH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THERMAL_MESH_NUM_NODES 9
#define THERMAL_MESH_MAX_SENSORS 4

/**
 * @brief 三维热网络节点枚举
 */
typedef enum {
    THERMAL_NODE_CORE_CENTER = 0,    /**< 电芯内部核心几何中心 (最高热点区域) */
    THERMAL_NODE_TAB_POSITIVE,       /**< 正极极耳及极柱引出端 (大电流收敛收缩热) */
    THERMAL_NODE_TAB_NEGATIVE,       /**< 负极极耳及极柱引出端 */
    THERMAL_NODE_TOP_SURFACE,        /**< 顶盖防爆阀区域 */
    THERMAL_NODE_BOTTOM_COOLING,     /**< 底部液冷板/导热垫贴合面 (主要散热边界) */
    THERMAL_NODE_SIDE_LEFT,          /**< 左侧窄面 */
    THERMAL_NODE_SIDE_RIGHT,         /**< 右侧窄面 */
    THERMAL_NODE_SURFACE_FRONT,      /**< 前侧大面表面中心 (常用表面贴片传感器贴附点) */
    THERMAL_NODE_SURFACE_BACK,       /**< 后侧大面表面中心 */
    THERMAL_NODE_MAX
} thermal_node_id_t;

/**
 * @brief 传感器绑定映射定义
 */
typedef struct {
    thermal_node_id_t mapped_node;   /**< 传感器贴附对应的节点 */
    float sensor_accuracy_c;         /**< 测量精度标准差 (℃, e.g. 0.5) */
    bool is_valid;                   /**< 传感器在线有效状态 */
} thermal_sensor_binding_t;

/**
 * @brief 热网络物性与几何参数配置
 */
typedef struct {
    float cell_mass_kg;              /**< 电芯质量 (kg, e.g. 5.4 for 280Ah LFP) */
    float specific_heat_j_kg_k;      /**< 比热容 Cp (J/(kg·K), e.g. 1100.0) */
    float k_in_plane_w_m_k;          /**< 面内 (平行极片) 导热系数 (W/(m·K), e.g. 28.0) */
    float k_cross_plane_w_m_k;       /**< 跨面 (垂直极片) 导热系数 (W/(m·K), e.g. 1.1) */
    float r_tab_pos_mohm;            /**< 正极极耳寄生内阻 (mΩ, e.g. 0.08) */
    float r_tab_neg_mohm;            /**< 负极极耳寄生内阻 (mΩ, e.g. 0.05) */
    float h_ambient_conv_w_k;        /**< 环境对流换热总导热率 (W/K, e.g. 0.4) */
    float h_liquid_plate_w_k;        /**< 底部液冷板换热总导热率 (W/K, e.g. 12.0) */
    float d_ocv_d_t_v_k;             /**< 熵热系数 dOCV/dT (V/K, e.g. -0.0002) */
    float max_temp_alarm_c;          /**< 核心热点超温告警阈值 (℃, e.g. 55.0) */
    float max_gradient_alarm_c;      /**< 核心-表面/最大温差梯度告警阈值 (℃, e.g. 8.0) */
} thermal_mesh_config_t;

/**
 * @brief 实时物理输入遥测采样
 */
typedef struct {
    float load_current_a;            /**< 电芯实时充放电电流 (A, 正为放电, 负为充电) */
    float cell_terminal_voltage_v;   /**< 电芯实时采样端电压 (V) */
    float open_circuit_voltage_v;    /**< 当前 SOC 对应 OCV 开路电压 (V) */
    float cell_soc;                  /**< 当前 SOC 状态 (0.0~1.0) */
    float ambient_temp_c;            /**< 箱体环境空气温度 (℃) */
    float coolant_temp_c;            /**< 底板液冷介质进水温度 (℃) */
    float sensor_temps[THERMAL_MESH_MAX_SENSORS]; /**< 实测表面传感器温度数组 (℃) */
    uint8_t sensor_count;            /**< 接入的实测温度传感器数量 (1..4) */
    thermal_sensor_binding_t bindings[THERMAL_MESH_MAX_SENSORS]; /**< 传感器安装物理映射 */
} thermal_mesh_telemetry_t;

/**
 * @brief 三维非均匀热场重建状态与物理热流输出
 */
typedef struct {
    float node_temperatures[THERMAL_MESH_NUM_NODES]; /**< 9 节点完整温度场 (℃) */
    float core_hotspot_temp_c;       /**< 内部核心热点最高温度 (℃) */
    thermal_node_id_t hotspot_node;  /**< 最高热点所在节点编号 */
    float min_temp_c;                /**< 最低温度节点温度 (℃) */
    float max_thermal_gradient_c;    /**< 电芯内部最大空间温度梯度 (℃) */
    float tab_gradient_c;            /**< 极耳至核心温差梯度 (℃) */
    float total_heat_gen_power_w;    /**< 总发热功率 (焦耳热+极耳热+极化热+熵热) (W) */
    float joule_heat_power_w;        /**< 欧姆发热功率 (W) */
    float polarization_heat_power_w; /**< 电化学过电位极化热功率 (W) */
    float reversible_entropic_heat_w;/**< 可逆熵热发热功率 (W) */
    float cooling_heat_flux_w;       /**< 导出到底部液冷板的散热热流 (W) */
    float ambient_loss_heat_w;       /**< 向周围空气散失的热流 (W) */
    bool hotspot_alarm;              /**< 核心热点超温告警标志 */
    bool gradient_alarm;             /**< 空间热梯度超标告警标志 */
} thermal_mesh_output_t;

/**
 * @brief 三维热阻抗降阶模型主引擎上下文
 */
typedef struct {
    thermal_mesh_config_t cfg;       /**< 模型物性配置 */
    float temperatures[THERMAL_MESH_NUM_NODES]; /**< 节点温度状态向量 */
    float heat_capacities[THERMAL_MESH_NUM_NODES]; /**< 各节点热容 C_i (J/K) */
    float conductance_matrix[THERMAL_MESH_NUM_NODES][THERMAL_MESH_NUM_NODES]; /**< 导热矩阵 G_ij (W/K) */
    float ambient_conductance[THERMAL_MESH_NUM_NODES]; /**< 对流换热导热率 (W/K) */
    float cooling_conductance[THERMAL_MESH_NUM_NODES]; /**< 液冷换热导热率 (W/K) */
    bool initialized;                /**< 初始化标志 */
} thermal_mesh_engine_t;

/**
 * @brief 初始化三维热阻抗矩阵重建引擎
 * @param engine 算法引擎句柄
 * @param cfg 配置参数 (传 NULL 则使用 280Ah LFP 工业默认参数)
 * @return 0 成功, 负数 错误码
 */
int thermal_mesh_engine_init(thermal_mesh_engine_t *engine, const thermal_mesh_config_t *cfg);

/**
 * @brief 重置热场状态向量 (例如停机冷态对齐环境温度)
 * @param engine 算法引擎句柄
 * @param init_temp_c 初始对齐温度 (℃)
 * @return 0 成功, 负数 错误码
 */
int thermal_mesh_engine_reset(thermal_mesh_engine_t *engine, float init_temp_c);

/**
 * @brief 周期步进热场动态重建计算 (推荐 10ms ~ 100ms 周期执行)
 * @param engine 算法引擎句柄
 * @param telem 实时输入采样
 * @param dt_s 物理步长 (秒)
 * @param out 重建输出结构
 * @return 0 成功, 负数 错误码
 */
int thermal_mesh_engine_step(thermal_mesh_engine_t *engine,
                             const thermal_mesh_telemetry_t *telem,
                             float dt_s,
                             thermal_mesh_output_t *out);

/**
 * @brief 获取热网络节点名称字符串
 * @param node 节点枚举
 * @return 节点名称
 */
const char *thermal_mesh_node_to_str(thermal_node_id_t node);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_THERMAL_MESH_H */
