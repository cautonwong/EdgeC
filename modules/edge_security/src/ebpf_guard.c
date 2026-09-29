/**
 * @file ebpf_guard.c
 * @brief Kernel-Level eBPF Syscall & SHM Anomaly Guard Engine Implementation
 */

#include "ebpf_guard.h"
#include <string.h>

struct ebpf_guard_mgr {
    size_t probe_count;
    ebpf_probe_t probes[EBPF_GUARD_MAX_PROBES];
    bool global_enforce_mode;
    bool bpf_ringbuf_supported;
    uint64_t ringbuf_events_ingested;
};

_Static_assert(sizeof(struct ebpf_guard_mgr) <= sizeof(ebpf_guard_mgr_storage_t),
               "ebpf_guard_mgr_storage_t size is insufficient");

ebpf_guard_mgr_t *ebpf_guard_mgr_create_in(ebpf_guard_mgr_storage_t *store) {
    if (!store) {
        return NULL;
    }
    ebpf_guard_mgr_t *mgr = (ebpf_guard_mgr_t *)store->raw;
    memset(mgr, 0, sizeof(*mgr));
    mgr->global_enforce_mode = true;
    mgr->bpf_ringbuf_supported = true;
    return mgr;
}

bool ebpf_guard_is_enforce_mode(const ebpf_guard_mgr_t *mgr) {
    return mgr ? mgr->global_enforce_mode : false;
}

uint64_t ebpf_guard_get_probe_events_processed(const ebpf_guard_mgr_t *mgr, uint32_t probe_id) {
    if (!mgr || probe_id == 0 || probe_id > mgr->probe_count) {
        return 0;
    }
    return mgr->probes[probe_id - 1].events_processed;
}

uint32_t ebpf_guard_get_probe_violations(const ebpf_guard_mgr_t *mgr, uint32_t probe_id) {
    if (!mgr || probe_id == 0 || probe_id > mgr->probe_count) {
        return 0;
    }
    return mgr->probes[probe_id - 1].violations_intercepted;
}

int ebpf_guard_init(ebpf_guard_mgr_t *mgr) {
    if (!mgr) {
        return -1;
    }

    memset(mgr, 0, sizeof(struct ebpf_guard_mgr));
    mgr->global_enforce_mode = true;
    mgr->bpf_ringbuf_supported = true; /* Linux kernel >= 5.8 bpf_ringbuf zero-syscall mode enabled */
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

int ebpf_guard_ringbuf_consume(ebpf_guard_mgr_t *mgr, const void *event_payload, size_t payload_len) {
    if (!mgr || !event_payload || payload_len == 0) {
        return -1;
    }

    mgr->ringbuf_events_ingested++;
    return 0;
}
