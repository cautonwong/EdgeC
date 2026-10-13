/**
 * @file iec104_apci.h
 * @brief IEC 104 应用规约控制信息 (APCI) 状态机定义
 */

#ifndef EDGE_IEC104_APCI_H
#define EDGE_IEC104_APCI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
typedef struct uv_loop_s uv_loop_t;

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

typedef struct iec104_apci_mgr iec104_apci_mgr_t;

/**
 * @brief IEC 104 APCI 管理器调用方分配存储容器 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[512];
} iec104_apci_mgr_storage_t;

/**
 * @brief 在调用方提供的内存就地构造 IEC 104 APCI 状态机聚合根 (Zero-Alloc)
 * @param store 调用方分配的存储块
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
iec104_apci_mgr_t *iec104_apci_mgr_create_in(iec104_apci_mgr_storage_t *store);

/**
 * @brief 获取与设置 APCI 状态
 */
iec104_state_t iec104_apci_get_state(const iec104_apci_mgr_t *mgr);
void iec104_apci_set_state(iec104_apci_mgr_t *mgr, iec104_state_t state);

/**
 * @brief 获取与设置序列号
 */
uint16_t iec104_apci_get_ns(const iec104_apci_mgr_t *mgr);
uint16_t iec104_apci_get_nr(const iec104_apci_mgr_t *mgr);
void iec104_apci_set_nr(iec104_apci_mgr_t *mgr, uint16_t nr);

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
