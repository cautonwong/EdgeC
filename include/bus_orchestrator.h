#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include "arbiter.h"
#include "datahub.h"
#include "edge_driver.h"

struct bus_orchestrator_s;
typedef struct bus_orchestrator_s bus_orchestrator_t;

/**
 * @brief 创建总线编排器 (集成 Arbiter)
 */
bus_orchestrator_t *bus_orchestrator_create(edge_driver_t *driver, datahub_t *hub, arbiter_t *arb);

/**
 * @brief 添加周期性轮询任务
 * @param ctx 编排器实例
 * @param interval_ms 轮询周期 (毫秒)
 * @param group 轮询组配置 (包含 slave_id, start_addr 等)
 */
int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, int interval_ms,
                              const edge_poll_group_t *group);

void bus_orchestrator_run(bus_orchestrator_t *ctx);
void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

#endif