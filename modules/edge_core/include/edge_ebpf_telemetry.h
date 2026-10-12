/**
 * @file edge_ebpf_telemetry.h
 * @brief eBPF Microsecond-Level Kernel Latency Telemetry & Adaptive Jitter Circuit Breaker (AEP-612)
 *
 * Provides real-time scheduling latency telemetry (P90/P99 jitter, hardirq disabled duration,
 * latency histogram), and a 4-state adaptive circuit breaker (Closed -> Warning -> Degraded -> Tripped)
 * to prevent hard real-time loop starvation in Linux PREEMPT_RT industrial edge deployments.
 *
 * Caller-allocated storage, zero dynamic allocation.
 */

#ifndef EDGE_EBPF_TELEMETRY_H
#define EDGE_EBPF_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_EBPF_HISTOGRAM_BINS 16

typedef enum {
    EBPF_CB_CLOSED   = 0, /* Normal operation: jitter within healthy bounds */
    EBPF_CB_WARNING  = 1, /* Warning: transient latency spikes observed */
    EBPF_CB_DEGRADED = 2, /* Degraded: background tasks throttled, critical tasks protected */
    EBPF_CB_TRIPPED  = 3  /* Tripped: non-critical background paused, CPU isolated */
} ebpf_cb_state_t;

typedef struct {
    uint32_t jitter_warn_us;               /* Jitter warning threshold in microseconds (e.g. 20 us) */
    uint32_t jitter_trip_us;               /* Jitter trip threshold in microseconds (e.g. 50 us) */
    uint32_t hardirq_trip_us;              /* Maximum allowable hardirq disabled duration (e.g. 100 us) */
    uint32_t consecutive_spike_threshold;  /* Number of consecutive spikes before advancing CB state */
    uint32_t recovery_healthy_cycles;      /* Consecutive healthy cycles required to step down CB state */
    uint32_t target_task_pid;              /* Monitored RT thread / process ID */
} ebpf_telemetry_config_t;

typedef struct {
    uint64_t sample_count;
    uint32_t max_jitter_us;
    uint32_t avg_jitter_us;
    uint32_t p90_jitter_us;
    uint32_t p99_jitter_us;
    uint32_t hardirq_off_max_us;
    uint32_t histogram_buckets[EDGE_EBPF_HISTOGRAM_BINS];
} ebpf_latency_stats_t;

typedef struct {
    ebpf_telemetry_config_t config;
    ebpf_latency_stats_t    stats;
    ebpf_cb_state_t         cb_state;
    uint32_t                consecutive_spikes;
    uint32_t                consecutive_healthy;
    uint32_t                trip_count;
    bool                    bpf_probe_active;
    bool                    user_fallback_active;
    bool                    non_critical_tasks_paused;
    uint32_t                isolated_cpu_mask;
    uint64_t                _total_jitter_acc;
    uint8_t                 _reserved[16];
} edge_ebpf_telemetry_ctx_t;

/**
 * @brief Initialize eBPF kernel latency telemetry context.
 */
int edge_ebpf_telemetry_init(edge_ebpf_telemetry_ctx_t *ctx, const ebpf_telemetry_config_t *cfg);

/**
 * @brief Record a kernel latency / scheduling jitter event.
 */
int edge_ebpf_telemetry_record_event(edge_ebpf_telemetry_ctx_t *ctx, uint32_t jitter_us, uint32_t hardirq_off_us);

/**
 * @brief Evaluate and transition the adaptive jitter circuit breaker state machine.
 */
int edge_ebpf_telemetry_evaluate_circuit_breaker(edge_ebpf_telemetry_ctx_t *ctx, ebpf_cb_state_t *out_state);

/**
 * @brief Retrieve current statistical latency summary.
 */
int edge_ebpf_telemetry_get_stats(const edge_ebpf_telemetry_ctx_t *ctx, ebpf_latency_stats_t *out_stats);

/**
 * @brief Reset statistics and clear trip states.
 */
int edge_ebpf_telemetry_reset(edge_ebpf_telemetry_ctx_t *ctx);

/**
 * @brief Synthetic kernel probe simulation helper for unit testing & CI without root bpf permissions.
 */
int edge_ebpf_telemetry_simulate_kernel_probe(edge_ebpf_telemetry_ctx_t *ctx, uint32_t synthetic_jitter_us, uint32_t synthetic_hardirq_us);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_EBPF_TELEMETRY_H */
