/**
 * @file dcu_heartbeat.h
 * @brief DCU 心跳：生产刷盘与心跳日志；演示冻结/遥控仅在 DCU_DEMO_MODE=1 下启用
 */

#ifndef DCU_HEARTBEAT_H
#define DCU_HEARTBEAT_H

#include <stdbool.h>
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
    int max_cycles;            /* 演示模式的自退周期，0 = 不退出 */
    bool demo_mode;            /* DCU_DEMO_MODE=1 时为真 */
    uint32_t demo_freezes;     /* 已推送的冻结触发数（测试可观测） */
    uint32_t demo_relays;      /* 已触发的 OI 8000 跳合闸数（测试可观测） */
    uint32_t control_triggers; /* 已触发的演示遥控数（测试可观测） */
    uint32_t stop_requested;   /* 进入自退路径的次数（测试可观测） */
    bool timers_inited;
    bool freeze_closed;
    bool control_closed;
} dcu_heartbeat_t;

/**
 * @brief 启动心跳定时器。
 *
 * 生产模式（缺省）：每 5 秒刷盘并记心跳日志，不冻结、不跳闸、不退出。
 * `DCU_DEMO_MODE=1` 时恢复台架行为：冻结管线、随机跳闸、遥控触发；
 * 此时 `DCU_HEARTBEAT_CYCLES` 控制自退周期（缺省 3，0 = 一直跑），
 * `DCU_CONTROL_DELAY_MS` 控制遥控延迟（缺省 2000ms）。
 */
void dcu_heartbeat_start(dcu_heartbeat_t *hb, uv_loop_t *loop, dcu_node_t *node,
                         stream_engine_t *se, service_t *svc);

/**
 * @brief 停止并关闭两个心跳定时器；幂等，拆除阶段必须在销毁 storage 前调用
 */
void dcu_heartbeat_stop(dcu_heartbeat_t *hb);

/** @brief 跑一次心跳周期；定时器回调与测试共用。 */
void dcu_heartbeat_tick(dcu_heartbeat_t *hb);

/** @brief 跑一次演示遥控触发；仅演示模式生效，定时器回调与测试共用。 */
void dcu_heartbeat_control_tick(dcu_heartbeat_t *hb);

#ifdef __cplusplus
}
#endif

#endif  // DCU_HEARTBEAT_H
