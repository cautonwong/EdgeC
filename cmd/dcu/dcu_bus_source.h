/**
 * @file dcu_bus_source.h
 * @brief DCU 真采集数据源：把 poller 的同步 read_value 接到异步总线上。
 *
 * 两阶段、跨轮次、同线程：
 *   第 1 次调用 → edge_api_read_tag() 把请求发出去（非阻塞），返回 POLL_READ_PENDING
 *   后续调用   → 从 shadow 按 (tag_id, meter_idx) 取值，命中即返回 0
 * 绝不把"还没读到"当成 0.0，也绝不阻塞主 loop。
 */

#ifndef DCU_BUS_SOURCE_H
#define DCU_BUS_SOURCE_H

#include <stdint.h>

#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dcu_bus_source_s dcu_bus_source_t;

/** @brief timeout_ms 必须小于轮询周期，否则永远等不到结果 */
dcu_bus_source_t *dcu_bus_source_start(datahub_t *hub, const dcu_inventory_t *inv,
                                       uint32_t timeout_ms);
void dcu_bus_source_stop(dcu_bus_source_t *bs);
poller_source_ops_t dcu_bus_source_ops(dcu_bus_source_t *bs);

/** @brief node 建好之后才能拿到 hub/inv，先建后绑 */

#ifdef __cplusplus
}
#endif

#endif
