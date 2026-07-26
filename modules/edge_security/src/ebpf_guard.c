/**
 * @file ebpf_guard.c
 * @brief Kernel-Level eBPF Syscall & SHM Anomaly Guard Engine Implementation
 */

#include "ebpf_guard.h"
#include <string.h>

int ebpf_guard_init(ebpf_guard_mgr_t *mgr) {
    if (!mgr) {
        return -1;
    }

    memset(mgr, 0, sizeof(ebpf_guard_mgr_t));
    mgr->global_enforce_mode = true;
    return 0;
}

int ebpf_guard_attach_probe(ebpf_guard_mgr_t *mgr, ebpf_probe_type_t type, uint32_t *out_probe_id) {
    if (!mgr || !out_probe_id) {
        return -1;
    }

    if (mgr->probe_count >= EBPF_GUARD_MAX_PROBES) {
        return -2;
    }

    uint32_t idx = (uint32_t)mgr->probe_count;
    mgr->probes[idx].probe_id = idx + 1;
    mgr->probes[idx].type = type;
    mgr->probes[idx].attached = true;
    mgr->probes[idx].events_processed = 0;
    mgr->probes[idx].violations_intercepted = 0;

    mgr->probe_count++;
    *out_probe_id = mgr->probes[idx].probe_id;
    return 0;
}

int ebpf_guard_audit_event(ebpf_guard_mgr_t *mgr, uint32_t probe_id, uint32_t pid, bool is_unauthorized_shm_mutation) {
    if (!mgr || probe_id == 0 || probe_id > mgr->probe_count) {
        return -1;
    }

    uint32_t idx = probe_id - 1;
    if (!mgr->probes[idx].attached) {
        return -2;
    }

    mgr->probes[idx].events_processed++;

    if (is_unauthorized_shm_mutation) {
        mgr->probes[idx].violations_intercepted++;
        return 1; /* Intercepted & Blocked */
    }

    return 0; /* Allowed */
}
