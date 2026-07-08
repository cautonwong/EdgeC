#ifndef EDGE_METRICS_H
#define EDGE_METRICS_H

#include <stdint.h>
#include "contract_data_types.h"
#include "datahub.h"

/**
 * @brief [ADR-0001] 注册一个系统 Metric 标签
 *
 * 模块在初始化时调用此函数注册自己负责的指标。
 * 注册后可通过 edge_metrics_report() 写入指标值。
 *
 * @param tag_id      标签 ID（必须在 EDGE_METRIC_TAG_BASE ~ EDGE_METRIC_TAG_MAX 之间）
 * @param name        指标名称（如 "datahub.ring.watermark"）
 * @param description 人类可读的描述
 * @return 0=成功, <0=错误
 */
int edge_metrics_register(uint32_t tag_id, const char *name, const char *description);

/**
 * @brief [ADR-0001] 报告一个 Metric 值
 *
 * 向 DataHub 写入一条 PIPEMSG_TYPE_METRIC 消息。
 * 高频环（100ms）写入实时指标（队列深度、延迟）。
 * 低频环（10s）写入诊断快照（内存、计数）。
 *
 * @param hub    DataHub 实例
 * @param tag_id 已注册的 Metric 标签 ID
 * @param value  当前值
 * @param ts     时间戳（毫秒），0 表示使用当前时间
 */
void edge_metrics_report(datahub_t *hub, uint32_t tag_id, double value, uint64_t ts);

/**
 * @brief [ADR-0001] 为给定模块声明高频环指标注册
 *
 * 在模块初始化时调用，注册该模块的所有高频指标。
 * 高频指标每 100ms 自动采集。
 */
int edge_metrics_register_fast(datahub_t *hub, uint32_t base_tag, uint32_t count);

/**
 * @brief [ADR-0001] 获取当前已注册的 Metric 标签数量
 */
uint32_t edge_metrics_count(void);

#endif
