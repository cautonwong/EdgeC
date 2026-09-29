/**
 * @file ebpf_guard.h
 * @brief Kernel-Level eBPF Syscall & SHM Anomaly Guard Engine
 */

#ifndef EBPF_GUARD_H
#define EBPF_GUARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#define EBPF_GUARD_MAX_PROBES 16

typedef enum {
    EBPF_PROBE_SYSCALL_MONITOR = 0,
    EBPF_PROBE_SHM_WRITE_AUDIT = 1,
    EBPF_PROBE_SOCK_REDIRECT = 2
} ebpf_probe_type_t;

typedef struct {
    uint32_t probe_id;
    ebpf_probe_type_t type;
    bool attached;
    uint64_t events_processed;
    uint32_t violations_intercepted;
} ebpf_probe_t;

typedef struct ebpf_guard_mgr ebpf_guard_mgr_t;

/**
 * @brief eBPF 监控管理器调用方分配存储块 (Caller-Allocated Storage - AEP-752)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[1024];
} ebpf_guard_mgr_storage_t;

/**
 * @brief 在调用方提供的内存就地构造 eBPF 安全监控聚合根 (Zero-Alloc)
 * @param store 调用方分配的存储块
 * @return 构造成功的聚合根指针, 失败返回 NULL
 */
ebpf_guard_mgr_t *ebpf_guard_mgr_create_in(ebpf_guard_mgr_storage_t *store);

/**
 * @brief 查询全局拦截模式
 */
bool ebpf_guard_is_enforce_mode(const ebpf_guard_mgr_t *mgr);

/**
 * @brief 查询指定探针已处理事件数
 */
uint64_t ebpf_guard_get_probe_events_processed(const ebpf_guard_mgr_t *mgr, uint32_t probe_id);

/**
 * @brief 查询指定探针已拦截违规数
 */
uint32_t ebpf_guard_get_probe_violations(const ebpf_guard_mgr_t *mgr, uint32_t probe_id);

/**
 * @brief Initializes eBPF security guard manager
 * @param mgr Pointer to eBPF guard manager instance
 * @return 0 on success, negative error code on failure
 */
int ebpf_guard_init(ebpf_guard_mgr_t *mgr);

/**
 * @brief Attaches a new kernel eBPF probe
 * @param mgr Pointer to eBPF guard manager instance
 * @param type Probe type
 * @param out_probe_id Output assigned probe ID
 * @return 0 on success, negative error code on failure
 */
int ebpf_guard_attach_probe(ebpf_guard_mgr_t *mgr, ebpf_probe_type_t type, uint32_t *out_probe_id);

/**
 * @brief Evaluates an incoming process syscall / SHM access event
 * @param mgr Pointer to eBPF guard manager instance
 * @param probe_id Probe ID
 * @param pid Process ID making access
 * @param is_unauthorized_shm_mutation True if unauthorized write attempt
 * @return 0 if allowed, 1 if intercepted violation, negative error code on failure
 */
int ebpf_guard_audit_event(ebpf_guard_mgr_t *mgr, uint32_t probe_id, uint32_t pid, bool is_unauthorized_shm_mutation);

/**
 * @brief Consumes events via bpf_ringbuf zero-syscall mechanism (or fallback)
 * @param mgr Pointer to eBPF guard manager instance
 * @param event_payload Event data buffer
 * @param payload_len Event buffer length
 * @return 0 on success, negative error code on failure
 */
int ebpf_guard_ringbuf_consume(ebpf_guard_mgr_t *mgr, const void *event_payload, size_t payload_len);

#ifdef __cplusplus
}
#endif

#endif /* EBPF_GUARD_H */
