/**
 * @file dcu_heartbeat.h
 * @brief DCU 演示/验收心跳：冻结管线触发、遥控触发、周期预算
 */

#ifndef DCU_HEARTBEAT_H
#define DCU_HEARTBEAT_H

#include <uv.h>

#include "datahub.h"
#include "dcu_node.h"
#include "edge_service.h"
#include "stream_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uv_loop_t *loop;
    dcu_node_t *node;
    stream_engine_t *se;
    service_t *svc;
    uv_timer_t freeze_timer;
    uv_timer_t control_timer;
    int test_cycles;
} dcu_heartbeat_t;

/**
 * @brief 启动冻结触发与遥控演示定时器。
 *
 * 心跳周期数由 `DCU_HEARTBEAT_CYCLES` 控制（缺省 3，0 = 一直跑）；
 * 遥控延迟由 `DCU_CONTROL_DELAY_MS` 控制（缺省 2000ms）。
 */
void dcu_heartbeat_start(dcu_heartbeat_t *hb, uv_loop_t *loop, dcu_node_t *node,
                         stream_engine_t *se, service_t *svc);

#ifdef __cplusplus
}
#endif

#endif  // DCU_HEARTBEAT_H
