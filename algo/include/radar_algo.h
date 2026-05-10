/**
 * @file radar_algo.h
 * @brief 工业级雷达信号处理算法 (CFAR 等)
 */

#ifndef EDGE_RADAR_ALGO_H
#define EDGE_RADAR_ALGO_H

#include <stdint.h>
#include <stddef.h>

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

#ifdef __cplusplus
}
#endif

#endif
