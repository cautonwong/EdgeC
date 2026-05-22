/**
 * @file iec104_apci.h
 * @brief IEC 104 应用规约控制信息 (APCI) 状态机定义
 */

#ifndef EDGE_IEC104_APCI_H
#define EDGE_IEC104_APCI_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IEC104_STATE_DISCONNECTED,
    IEC104_STATE_CONNECTING,
    IEC104_STATE_STARTING,  /**< 发送了 STARTDT ACT */
    IEC104_STATE_CONNECTED, /**< 收到 STARTDT CON，可以传输数据 */
    IEC104_STATE_STOPPING   /**< 发送了 STOPDT ACT */
} iec104_state_t;

/**
 * @brief APCI 管理器上下文
 */
typedef struct {
    iec104_state_t state;

    // 序列号维护
    uint16_t ns;       /**< 发送序列号 N(S) */
    uint16_t nr;       /**< 接收序列号 N(R) */
    uint16_t acked_nr; /**< 上次对方确认到的序列号 */

    // 滑窗控制 (k, w 因子)
    uint16_t k; /**< 最大未确认发送 I 帧数 (默认 12) */
    uint16_t w; /**< 最大未确认接收 I 帧数 (默认 8) */

    // 定时器 (t0, t1, t2, t3)
    uint64_t t1_start_ts; /**< I 帧发送时间戳 (用于 t1 超时监测) */
    uint64_t t2_start_ts; /**< 未确认接收 I 帧时间戳 (用于 t2 自动发 S 帧) */
    uint64_t t3_start_ts; /**< 空闲链路时间戳 (用于 t3 发送 TESTFR) */

    // 发送备份队列 (用于重发，逻辑上暂由 Engine 协助)
    net_buf_t *unacked_frames[32];

} iec104_apci_mgr_t;

/**
 * @brief 初始化 APCI 管理器
 */
void iec104_apci_init(iec104_apci_mgr_t *mgr);

/**
 * @brief 构建 U 帧 (STARTDT, STOPDT, TESTFR)
 */
typedef enum {
    IEC104_U_STARTDT_ACT,
    IEC104_U_STARTDT_CON,
    IEC104_U_STOPDT_ACT,
    IEC104_U_STOPDT_CON,
    IEC104_U_TESTFR_ACT,
    IEC104_U_TESTFR_CON
} iec104_u_type_t;

int iec104_apci_build_u(iec104_u_type_t type, net_buf_t *out);

/**
 * @brief 构建 S 帧 (确认帧)
 */
int iec104_apci_build_s(iec104_apci_mgr_t *mgr, net_buf_t *out);

/**
 * @brief 处理收到的确认号 N(R)
 */
void iec104_apci_handle_nr(iec104_apci_mgr_t *mgr, uint16_t nr);

/**
 * @brief 在 I 帧发送前执行 APCI 封包 (Zero-Copy)
 */
int iec104_apci_wrap_i(iec104_apci_mgr_t *mgr, net_buf_t *asdu);

/**
 * @brief APCI 状态机反馈事件
 */
typedef enum {
    IEC104_EVENT_NONE,
    IEC104_EVENT_SEND_S,      /**< 需要发送 S 帧确认 (t2 到期) */
    IEC104_EVENT_SEND_TESTFR, /**< 需要发送 TESTFR (t3 到期) */
    IEC104_EVENT_T1_TIMEOUT,  /**< 发送确认超时 (t1 到期) */
    IEC104_EVENT_T0_TIMEOUT   /**< 连接建立超时 (t0 到期) */
} iec104_apci_event_t;

/**
 * @brief 周期性状态监测 (由 Engine 调用)
 * @param now 当前时间戳 (ms)
 */
iec104_apci_event_t iec104_apci_tick(iec104_apci_mgr_t *mgr, uint64_t now);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_IEC104_APCI_H
