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

typedef struct {
    size_t probe_count;
    ebpf_probe_t probes[EBPF_GUARD_MAX_PROBES];
    bool global_enforce_mode;
} ebpf_guard_mgr_t;

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

#ifdef __cplusplus
}
#endif

#endif /* EBPF_GUARD_H */
