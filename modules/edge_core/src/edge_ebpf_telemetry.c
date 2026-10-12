/**
 * @file edge_ebpf_telemetry.c
 * @brief eBPF Microsecond-Level Kernel Latency Telemetry & Adaptive Jitter Circuit Breaker Implementation (AEP-612)
 */

#include "edge_ebpf_telemetry.h"
#include <string.h>

/* Histogram boundaries in microseconds:
 * 0: <5us, 1: <10us, 2: <20us, 3: <30us, 4: <50us, 5: <75us, 6: <100us, 7: <150us,
 * 8: <200us, 9: <300us, 10: <500us, 11: <1000us, 12: <2000us, 13: <5000us, 14: <10000us, 15: >=10000us
 */
static const uint32_t _histogram_bounds[EDGE_EBPF_HISTOGRAM_BINS - 1] = {
    5, 10, 20, 30, 50, 75, 100, 150, 200, 300, 500, 1000, 2000, 5000, 10000
};

static inline int _get_histogram_bin(uint32_t val_us) {
    for (int i = 0; i < EDGE_EBPF_HISTOGRAM_BINS - 1; i++) {
        if (val_us < _histogram_bounds[i]) {
            return i;
        }
    }
    return EDGE_EBPF_HISTOGRAM_BINS - 1;
}

int edge_ebpf_telemetry_init(edge_ebpf_telemetry_ctx_t *ctx, const ebpf_telemetry_config_t *cfg) {
    if (!ctx) return -1;

    memset(ctx, 0, sizeof(*ctx));

    if (cfg) {
        ctx->config = *cfg;
    } else {
        ctx->config.jitter_warn_us = 20;
        ctx->config.jitter_trip_us = 50;
        ctx->config.hardirq_trip_us = 100;
        ctx->config.consecutive_spike_threshold = 3;
        ctx->config.recovery_healthy_cycles = 10;
        ctx->config.target_task_pid = 0;
    }

    if (ctx->config.jitter_warn_us == 0) ctx->config.jitter_warn_us = 20;
    if (ctx->config.jitter_trip_us == 0) ctx->config.jitter_trip_us = 50;
    if (ctx->config.hardirq_trip_us == 0) ctx->config.hardirq_trip_us = 100;
    if (ctx->config.consecutive_spike_threshold == 0) ctx->config.consecutive_spike_threshold = 3;
    if (ctx->config.recovery_healthy_cycles == 0) ctx->config.recovery_healthy_cycles = 10;

    ctx->cb_state = EBPF_CB_CLOSED;
    ctx->bpf_probe_active = true;
    ctx->user_fallback_active = false;
    ctx->non_critical_tasks_paused = false;
    ctx->isolated_cpu_mask = 0x01; /* Default CPU0 */

    return 0;
}

int edge_ebpf_telemetry_record_event(edge_ebpf_telemetry_ctx_t *ctx, uint32_t jitter_us, uint32_t hardirq_off_us) {
    if (!ctx) return -1;

    ctx->stats.sample_count++;
    ctx->_total_jitter_acc += jitter_us;
    ctx->stats.avg_jitter_us = (uint32_t)(ctx->_total_jitter_acc / ctx->stats.sample_count);

    if (jitter_us > ctx->stats.max_jitter_us) {
        ctx->stats.max_jitter_us = jitter_us;
    }
    if (hardirq_off_us > ctx->stats.hardirq_off_max_us) {
        ctx->stats.hardirq_off_max_us = hardirq_off_us;
    }

    int bin = _get_histogram_bin(jitter_us);
    ctx->stats.histogram_buckets[bin]++;

    /* Compute P90 and P99 percentiles from histogram */
    uint64_t total = ctx->stats.sample_count;
    uint64_t target_p90 = (total * 90) / 100;
    uint64_t target_p99 = (total * 99) / 100;
    uint64_t cum = 0;
    bool found_p90 = false;
    bool found_p99 = false;

    for (int i = 0; i < EDGE_EBPF_HISTOGRAM_BINS; i++) {
        cum += ctx->stats.histogram_buckets[i];
        if (!found_p90 && cum >= target_p90) {
            ctx->stats.p90_jitter_us = (i < EDGE_EBPF_HISTOGRAM_BINS - 1) ? _histogram_bounds[i] : 10000;
            found_p90 = true;
        }
        if (!found_p99 && cum >= target_p99) {
            ctx->stats.p99_jitter_us = (i < EDGE_EBPF_HISTOGRAM_BINS - 1) ? _histogram_bounds[i] : 10000;
            found_p99 = true;
            break;
        }
    }

    /* Check for spike condition */
    bool is_spike = (jitter_us >= ctx->config.jitter_trip_us) ||
                    (hardirq_off_us >= ctx->config.hardirq_trip_us);
    bool is_warn = (jitter_us >= ctx->config.jitter_warn_us);

    if (is_spike) {
        ctx->consecutive_spikes++;
        ctx->consecutive_healthy = 0;
    } else if (is_warn) {
        ctx->consecutive_spikes++;
        ctx->consecutive_healthy = 0;
    } else {
        ctx->consecutive_healthy++;
        if (ctx->consecutive_spikes > 0) {
            ctx->consecutive_spikes--;
        }
    }

    /* Circuit Breaker State Transition Evaluation */
    if (ctx->consecutive_spikes >= ctx->config.consecutive_spike_threshold) {
        if (is_spike && ctx->cb_state < EBPF_CB_TRIPPED) {
            ctx->cb_state = EBPF_CB_TRIPPED;
            ctx->non_critical_tasks_paused = true;
            ctx->trip_count++;
        } else if (is_warn && ctx->cb_state < EBPF_CB_DEGRADED) {
            ctx->cb_state = EBPF_CB_DEGRADED;
            ctx->non_critical_tasks_paused = true;
        }
    } else if (is_warn && ctx->cb_state == EBPF_CB_CLOSED) {
        ctx->cb_state = EBPF_CB_WARNING;
    }

    /* Recovery transition */
    if (ctx->consecutive_healthy >= ctx->config.recovery_healthy_cycles) {
        if (ctx->cb_state == EBPF_CB_TRIPPED) {
            ctx->cb_state = EBPF_CB_DEGRADED;
            ctx->consecutive_healthy = 0;
        } else if (ctx->cb_state == EBPF_CB_DEGRADED) {
            ctx->cb_state = EBPF_CB_WARNING;
            ctx->non_critical_tasks_paused = false;
            ctx->consecutive_healthy = 0;
        } else if (ctx->cb_state == EBPF_CB_WARNING) {
            ctx->cb_state = EBPF_CB_CLOSED;
            ctx->non_critical_tasks_paused = false;
            ctx->consecutive_healthy = 0;
        }
    }

    return 0;
}

int edge_ebpf_telemetry_evaluate_circuit_breaker(edge_ebpf_telemetry_ctx_t *ctx, ebpf_cb_state_t *out_state) {
    if (!ctx) return -1;
    if (out_state) {
        *out_state = ctx->cb_state;
    }
    return 0;
}

int edge_ebpf_telemetry_get_stats(const edge_ebpf_telemetry_ctx_t *ctx, ebpf_latency_stats_t *out_stats) {
    if (!ctx || !out_stats) return -1;
    *out_stats = ctx->stats;
    return 0;
}

int edge_ebpf_telemetry_reset(edge_ebpf_telemetry_ctx_t *ctx) {
    if (!ctx) return -1;
    memset(&ctx->stats, 0, sizeof(ctx->stats));
    ctx->cb_state = EBPF_CB_CLOSED;
    ctx->consecutive_spikes = 0;
    ctx->consecutive_healthy = 0;
    ctx->non_critical_tasks_paused = false;
    ctx->_total_jitter_acc = 0;
    return 0;
}

int edge_ebpf_telemetry_simulate_kernel_probe(edge_ebpf_telemetry_ctx_t *ctx, uint32_t synthetic_jitter_us, uint32_t synthetic_hardirq_us) {
    return edge_ebpf_telemetry_record_event(ctx, synthetic_jitter_us, synthetic_hardirq_us);
}
