/**
 * @file custom_soc_filter.h
 * @brief Secondary Developer Example: Custom Battery SOC Estimation & Smoothing Algorithm
 */

#ifndef EXAMPLES_CUSTOM_SOC_FILTER_H
#define EXAMPLES_CUSTOM_SOC_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <edgeos/edgeos.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float alpha;         /**< 平滑衰减因子 (0.0 < alpha <= 1.0) */
    float filtered_soc;  /**< 当前平滑后的 SOC 估计值 (%) */
    bool initialized;
} custom_soc_filter_t;

/**
 * @brief 初始化 SOC 滤波器
 */
int custom_soc_filter_init(custom_soc_filter_t *filter, float alpha, float initial_soc);

/**
 * @brief 执行单步滤波计算
 */
float custom_soc_filter_step(custom_soc_filter_t *filter, float raw_soc);

/**
 * @brief 从 DataHub 影子总线读取原始测点，执行滤波并写回目标测点
 */
int custom_soc_filter_process_datahub(custom_soc_filter_t *filter, edgeos_datahub_t *hub, uint32_t raw_tag_id, uint32_t out_tag_id);

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLES_CUSTOM_SOC_FILTER_H */
