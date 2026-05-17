/**
 * @file radar_algo.h
 * @brief 工业级雷达信号处理算法 (CFAR 等)
 */

#ifndef EDGE_RADAR_ALGO_H
#define EDGE_RADAR_ALGO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CA-CFAR (单元平均恒虚警) 算法
 * @param data 输入功率谱 (或幅值)
 * @param n 数据长度
 * @param train_cells 训练单元数量 (一侧)
 * @param guard_cells 保护单元数量 (一侧)
 * @param alpha 阈值系数
 * @param peaks 输出检测到的峰值索引 (数组，长度 n，1表示检测到，0表示无)
 * @return 检测到的目标数量
 */
int radar_cfar_ca(const double *data, size_t n, uint32_t train_cells, 
                  uint32_t guard_cells, double alpha, uint8_t *peaks);

/* --- 2. 目标跟踪 (Multi-Target Tracking) --- */

typedef struct {
    uint32_t id;
    double x, y;
    double vx, vy;
    double p_cov[4][4]; // 状态协方差
    uint32_t age;
    uint32_t hits;
    bool active;
} radar_target_t;

/**
 * @brief 更新雷达追踪器状态 (使用 GNN 关联 + KF 更新)
 * @param targets 当前已存在的追踪目标数组
 * @param target_count 目标数组长度
 * @param detections 当前帧探测到的新点迹 (x, y 数组)
 * @param det_count 新点迹数量
 * @param dt 采样间隔
 */
int radar_tracker_update(radar_target_t *targets, uint32_t max_targets,
                         const double *detections_x, const double *detections_y,
                         uint32_t det_count, double dt);

#ifdef __cplusplus
}
#endif

#endif
