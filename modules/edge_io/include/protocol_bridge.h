#ifndef PROTOCOL_BRIDGE_H
#define PROTOCOL_BRIDGE_H

#include <uv.h>
#include "bus_orchestrator.h"
#include "datahub.h"

typedef struct protocol_bridge_s protocol_bridge_t;

/**
 * @brief 创建协议桥接器
 */
protocol_bridge_t *protocol_bridge_create(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch);

/**
 * @brief 启动桥接逻辑 (订阅 DataHub 并开始路由)
 */
int protocol_bridge_start(protocol_bridge_t *bridge);

void protocol_bridge_destroy(protocol_bridge_t *bridge);

#endif
