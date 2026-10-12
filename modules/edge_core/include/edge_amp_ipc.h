/**
 * @file edge_amp_ipc.h
 * @brief Heterogeneous Multi-Core IPC & Zero-Copy SHM Ring Engine (AEP-605)
 *
 * Implements:
 * 1. Lock-free SPSC inter-core shared memory ringbuffer (Carveout RAM).
 * 2. High-throughput telemetry and control point streaming (>= 100k msg/s).
 * 3. RemoteProc heartbeat watchdog & <500ms auto-recovery state machine.
 */

#ifndef EDGE_AMP_IPC_H
#define EDGE_AMP_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMP_IPC_MAX_PAYLOAD     64
#define AMP_IPC_RING_CAPACITY   128
#define AMP_HEARTBEAT_TIMEOUT_MS 50
#define AMP_RELOAD_MAX_TIME_MS  500

/**
 * @brief AMP 核间消息槽位
 */
typedef struct {
    uint32_t seq;
    uint32_t tag_id;
    uint64_t timestamp_ns;
    uint32_t payload_len;
    uint8_t payload[AMP_IPC_MAX_PAYLOAD];
    uint32_t checksum;
} amp_ipc_msg_t;

/**
 * @brief 共享内存无锁单生产者单消费者 (SPSC) 环形缓冲区结构
 */
typedef struct {
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t capacity;
    amp_ipc_msg_t slots[AMP_IPC_RING_CAPACITY];
} amp_shm_ring_t;

/**
 * @brief AMP 核心运行状态
 */
typedef enum {
    AMP_CORE_STATE_OFFLINE = 0,
    AMP_CORE_STATE_RUNNING = 1,
    AMP_CORE_STATE_FAULT = 2,
    AMP_CORE_STATE_RELOADING = 3,
    AMP_CORE_STATE_RECOVERED = 4
} amp_core_state_t;

/**
 * @brief AMP 遥测统计指标
 */
typedef struct {
    uint64_t msgs_sent;
    uint64_t msgs_received;
    uint64_t msgs_dropped;
    uint64_t checksum_errors;
    uint64_t heartbeats_received;
    uint64_t heartbeat_timeouts;
    uint64_t reload_events;
    uint64_t max_latency_ns;
} amp_telemetry_t;

/**
 * @brief AMP 引擎主结构 (Caller-Allocated)
 */
typedef struct {
    amp_shm_ring_t *tx_ring;  /* A -> M 发送环 */
    amp_shm_ring_t *rx_ring;  /* M -> A 接收环 */
    amp_shm_ring_t internal_tx_storage;
    amp_shm_ring_t internal_rx_storage;
    amp_core_state_t remote_core_state;
    uint64_t last_heartbeat_time_ms;
    uint64_t reload_start_time_ms;
    uint32_t next_tx_seq;
    uint32_t next_rx_seq;
    amp_telemetry_t telemetry;
} edge_amp_ipc_t;

/**
 * @brief 初始化 AMP IPC 引擎
 */
int edge_amp_ipc_init(edge_amp_ipc_t *ipc, void *shm_base, size_t shm_size);

/**
 * @brief 发送跨核消息 (写入 TX Ring)
 */
int edge_amp_ipc_send(edge_amp_ipc_t *ipc, uint32_t tag_id, const void *payload, uint32_t len, uint64_t timestamp_ns);

/**
 * @brief 接收跨核消息 (从 RX Ring 读取)
 */
int edge_amp_ipc_recv(edge_amp_ipc_t *ipc, amp_ipc_msg_t *out_msg);

/**
 * @brief 处理远程实时核心跳
 */
int edge_amp_ipc_heartbeat_tick(edge_amp_ipc_t *ipc, uint64_t current_time_ms);

/**
 * @brief 驱动看门狗检测与自愈重载状态机
 * @param current_time_ms 当前毫秒时间戳
 * @param out_reloaded 若发生并完成重载返回 true
 * @return 0 on success, <0 on error
 */
int edge_amp_ipc_poll_watchdog(edge_amp_ipc_t *ipc, uint64_t current_time_ms, bool *out_reloaded);

/**
 * @brief 获取 AMP IPC 遥测统计
 */
void edge_amp_ipc_get_telemetry(const edge_amp_ipc_t *ipc, amp_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AMP_IPC_H */
