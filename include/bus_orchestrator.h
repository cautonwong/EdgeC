#ifndef BUS_ORCHESTRATOR_H
#define BUS_ORCHESTRATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "edge_driver.h"
#include "datahub.h"
#include "arbiter.h"

typedef struct bus_orchestrator_s bus_orchestrator_t;

/**
 * @brief 创建总线编排器
 */
bus_orchestrator_t *bus_orchestrator_create(datahub_t *hub, arbiter_t *arb, 
                                            int max_buses, uint32_t pool_size, uint32_t buf_size);

/**
 * @brief 绑定物理 FD 到总线 ID
 */
void bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd);

/**
 * @brief 添加 V3 状态化轮询任务
 */
int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, const char *protocol, uint32_t device_id, uint32_t tag_id, int interval_ms);

void bus_orchestrator_run(bus_orchestrator_t *ctx);
void bus_orchestrator_destroy(bus_orchestrator_t *ctx);

#endif
