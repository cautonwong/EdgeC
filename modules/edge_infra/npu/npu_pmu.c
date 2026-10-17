/**
 * @file npu_pmu.c
 * @brief Implementation of NPU PMU Hardware Performance Monitoring Unit (AEP-748 / Issue #278)
 */

#include "npu_pmu.h"
#include "datahub.h"

#include <string.h>
#include <time.h>
#include <math.h>

/* Tag IDs from contracts */
#define TAG_NPU_PMU_EXEC_CYCLES     262151
#define TAG_NPU_PMU_CACHE_MISS_PCT  262152
#define TAG_NPU_PMU_MAC_UTIL_PCT    262153

/* -------------------------------------------------------------------------- */
/* Internal Data Structures                                                   */
/* -------------------------------------------------------------------------- */

typedef struct {
    uint32_t op_id;
    npu_op_type_t op_type;
    char op_name[NPU_PMU_MAX_OP_NAME_LEN];
    uint64_t total_latency_ns;
    uint64_t total_cycles;
    uint64_t total_instructions;
    uint64_t total_l1_hits;
    uint64_t total_l1_misses;
    uint64_t total_l2_hits;
    uint64_t total_l2_misses;
    uint32_t count;
    double sum_latency_sq;                       /**< Sum of squares for variance / sigma */
    uint64_t max_latency_ns;
    uint64_t min_latency_ns;
} op_accumulator_t;

typedef struct {
    uint64_t token;
    uint32_t op_id;
    npu_op_type_t op_type;
    char op_name[NPU_PMU_MAX_OP_NAME_LEN];
    uint64_t start_time_ns;
    uint64_t expected_macs;
    size_t memory_bytes;
    bool active;
} active_session_t;

typedef struct npu_pmu_internal {
    npu_pmu_config_t config;
    active_session_t active_op;
    uint64_t next_token;

    /* Ring buffer of recent records */
    npu_pmu_record_t ring_buffer[NPU_PMU_RING_CAPACITY];
    uint32_t ring_head;
    uint32_t ring_count;

    /* Per-operator aggregated history */
    op_accumulator_t op_stats[NPU_PMU_MAX_TRACKED_OPS];
    uint32_t tracked_op_count;

    /* Global summary */
    uint64_t total_inferences;
    uint64_t total_ops_profiled;
    uint64_t total_cycles;
    uint64_t total_latency_ns;
    uint32_t jitter_anomaly_count;
} npu_pmu_internal_t;

_Static_assert(sizeof(npu_pmu_internal_t) <= sizeof(npu_pmu_storage_t),
               "npu_pmu_storage_t buffer size is insufficient for internal state");

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

static uint64_t get_time_ns_raw(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static op_accumulator_t *find_or_create_accumulator(npu_pmu_internal_t *self, uint32_t op_id,
                                                    npu_op_type_t op_type, const char *op_name) {
    for (uint32_t i = 0; i < self->tracked_op_count; i++) {
        if (self->op_stats[i].op_id == op_id) {
            return &self->op_stats[i];
        }
    }

    if (self->tracked_op_count < NPU_PMU_MAX_TRACKED_OPS) {
        op_accumulator_t *acc = &self->op_stats[self->tracked_op_count++];
        memset(acc, 0, sizeof(*acc));
        acc->op_id = op_id;
        acc->op_type = op_type;
        if (op_name) {
            strncpy(acc->op_name, op_name, sizeof(acc->op_name) - 1);
            acc->op_name[sizeof(acc->op_name) - 1] = '\0';
        }
        acc->min_latency_ns = UINT64_MAX;
        return acc;
    }

    /* Fallback to slot 0 if full */
    return &self->op_stats[0];
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

npu_pmu_t *npu_pmu_create_in(npu_pmu_storage_t *storage, const npu_pmu_config_t *config) {
    if (!storage) {
        return NULL;
    }

    npu_pmu_internal_t *self = (npu_pmu_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));

    if (config) {
        self->config = *config;
    } else {
        self->config.peak_macs_per_cycle = 512;
        self->config.core_clock_ghz = 1.0f;
        self->config.jitter_threshold_sigma = 3.0f;
        self->config.enable_ring_buffer = true;
    }

    if (self->config.peak_macs_per_cycle == 0) {
        self->config.peak_macs_per_cycle = 512;
    }
    if (self->config.core_clock_ghz <= 0.0f) {
        self->config.core_clock_ghz = 1.0f;
    }
    if (self->config.jitter_threshold_sigma <= 0.0f) {
        self->config.jitter_threshold_sigma = 3.0f;
    }

    self->next_token = 1;
    return (npu_pmu_t *)self;
}

uint64_t npu_pmu_op_begin(npu_pmu_t *pmu, uint32_t op_id, npu_op_type_t op_type,
                          const char *op_name, uint64_t expected_macs, size_t memory_bytes) {
    if (!pmu) {
        return 0;
    }

    npu_pmu_internal_t *self = (npu_pmu_internal_t *)pmu;
    uint64_t token = self->next_token++;
    if (self->next_token == 0) {
        self->next_token = 1;
    }

    self->active_op.token = token;
    self->active_op.op_id = op_id;
    self->active_op.op_type = op_type;
    if (op_name) {
        strncpy(self->active_op.op_name, op_name, sizeof(self->active_op.op_name) - 1);
        self->active_op.op_name[sizeof(self->active_op.op_name) - 1] = '\0';
    } else {
        snprintf(self->active_op.op_name, sizeof(self->active_op.op_name), "op_%u", op_id);
    }
    self->active_op.expected_macs = expected_macs;
    self->active_op.memory_bytes = memory_bytes;
    self->active_op.start_time_ns = get_time_ns_raw();
    self->active_op.active = true;

    return token;
}

int npu_pmu_op_end(npu_pmu_t *pmu, uint64_t token,
                   uint64_t l1_hits, uint64_t l1_misses,
                   uint64_t l2_hits, uint64_t l2_misses,
                   npu_pmu_record_t *out_record) {
    if (!pmu) {
        return -1;
    }

    npu_pmu_internal_t *self = (npu_pmu_internal_t *)pmu;
    if (!self->active_op.active || self->active_op.token != token) {
        return -2;
    }

    uint64_t now_ns = get_time_ns_raw();
    uint64_t start_ns = self->active_op.start_time_ns;
    uint64_t latency_ns = (now_ns >= start_ns) ? (now_ns - start_ns) : 1;
    if (latency_ns == 0) latency_ns = 1;

    /* Compute cycles based on clock frequency */
    double duration_sec = (double)latency_ns / 1e9;
    uint64_t cycles = (uint64_t)(duration_sec * (double)self->config.core_clock_ghz * 1e9);
    if (cycles == 0) cycles = 1;

    npu_pmu_record_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.op_id = self->active_op.op_id;
    rec.op_type = self->active_op.op_type;
    strncpy(rec.op_name, self->active_op.op_name, sizeof(rec.op_name) - 1);
    rec.start_time_ns = start_ns;
    rec.end_time_ns = now_ns;
    rec.latency_ns = latency_ns;
    rec.cycles = cycles;
    rec.instructions = self->active_op.expected_macs;

    rec.ipc = (cycles > 0) ? ((float)rec.instructions / (float)cycles) : 0.0f;

    rec.l1_hits = l1_hits;
    rec.l1_misses = l1_misses;
    uint64_t l1_total = l1_hits + l1_misses;
    rec.l1_miss_pct = (l1_total > 0) ? ((float)l1_misses / (float)l1_total * 100.0f) : 0.0f;

    rec.l2_hits = l2_hits;
    rec.l2_misses = l2_misses;
    uint64_t l2_total = l2_hits + l2_misses;
    rec.l2_miss_pct = (l2_total > 0) ? ((float)l2_misses / (float)l2_total * 100.0f) : 0.0f;

    /* Compute MAC utilization: actual MACs / (cycles * peak_macs_per_cycle) */
    uint64_t peak_capacity_macs = cycles * (uint64_t)self->config.peak_macs_per_cycle;
    if (peak_capacity_macs > 0) {
        float util = ((float)rec.instructions / (float)peak_capacity_macs) * 100.0f;
        rec.mac_util_pct = (util > 100.0f) ? 100.0f : util;
    } else {
        rec.mac_util_pct = 0.0f;
    }

    /* Effective memory bandwidth in GB/s */
    if (duration_sec > 0.0) {
        rec.memory_bw_gbps = (float)((double)self->active_op.memory_bytes / duration_sec / 1e9);
    } else {
        rec.memory_bw_gbps = 0.0f;
    }

    /* Update Accumulator and Check 3-Sigma Anomaly */
    op_accumulator_t *acc = find_or_create_accumulator(self, rec.op_id, rec.op_type, rec.op_name);
    if (acc->count >= 5) {
        double mean = (double)acc->total_latency_ns / (double)acc->count;
        double variance = (acc->sum_latency_sq / (double)acc->count) - (mean * mean);
        if (variance > 0.0) {
            double std_dev = sqrt(variance);
            double sigma_thresh = mean + ((double)self->config.jitter_threshold_sigma * std_dev);
            double min_jump_thresh = mean * 2.0; /* At least 100% (2x) jump to avoid OS scheduling noise */
            double threshold = (sigma_thresh > min_jump_thresh) ? sigma_thresh : min_jump_thresh;
            if ((double)latency_ns > threshold) {
                rec.is_jitter_anomaly = true;
                self->jitter_anomaly_count++;
            }
        }
    }

    acc->total_latency_ns += latency_ns;
    acc->total_cycles += cycles;
    acc->total_instructions += rec.instructions;
    acc->total_l1_hits += l1_hits;
    acc->total_l1_misses += l1_misses;
    acc->total_l2_hits += l2_hits;
    acc->total_l2_misses += l2_misses;
    acc->count++;
    acc->sum_latency_sq += (double)latency_ns * (double)latency_ns;
    if (latency_ns > acc->max_latency_ns) acc->max_latency_ns = latency_ns;
    if (latency_ns < acc->min_latency_ns) acc->min_latency_ns = latency_ns;

    /* Update Global Counters */
    self->total_ops_profiled++;
    self->total_cycles += cycles;
    self->total_latency_ns += latency_ns;

    /* Push to Ring Buffer */
    if (self->config.enable_ring_buffer) {
        self->ring_buffer[self->ring_head] = rec;
        self->ring_head = (self->ring_head + 1) % NPU_PMU_RING_CAPACITY;
        if (self->ring_count < NPU_PMU_RING_CAPACITY) {
            self->ring_count++;
        }
    }

    self->active_op.active = false;

    if (out_record) {
        *out_record = rec;
    }
    return 0;
}

int npu_pmu_get_stats(const npu_pmu_t *pmu, npu_pmu_stats_t *out_stats) {
    if (!pmu || !out_stats) {
        return -1;
    }

    const npu_pmu_internal_t *self = (const npu_pmu_internal_t *)pmu;
    memset(out_stats, 0, sizeof(*out_stats));

    out_stats->total_ops_profiled = self->total_ops_profiled;
    out_stats->total_cycles = self->total_cycles;
    out_stats->total_latency_ns = self->total_latency_ns;
    out_stats->jitter_anomaly_count = self->jitter_anomaly_count;

    if (self->total_ops_profiled > 0) {
        out_stats->avg_inference_latency_us = (float)((double)self->total_latency_ns / (double)self->total_ops_profiled / 1000.0);
    }

    uint64_t total_l1_hits = 0, total_l1_miss = 0;
    uint64_t total_l2_hits = 0, total_l2_miss = 0;
    uint64_t total_macs = 0;

    for (uint32_t i = 0; i < self->tracked_op_count; i++) {
        total_l1_hits += self->op_stats[i].total_l1_hits;
        total_l1_miss += self->op_stats[i].total_l1_misses;
        total_l2_hits += self->op_stats[i].total_l2_hits;
        total_l2_miss += self->op_stats[i].total_l2_misses;
        total_macs += self->op_stats[i].total_instructions;
    }

    uint64_t l1_all = total_l1_hits + total_l1_miss;
    out_stats->avg_l1_miss_pct = (l1_all > 0) ? ((float)total_l1_miss / (float)l1_all * 100.0f) : 0.0f;

    uint64_t l2_all = total_l2_hits + total_l2_miss;
    out_stats->avg_l2_miss_pct = (l2_all > 0) ? ((float)total_l2_miss / (float)l2_all * 100.0f) : 0.0f;

    uint64_t peak_capacity = self->total_cycles * (uint64_t)self->config.peak_macs_per_cycle;
    if (peak_capacity > 0) {
        float util = ((float)total_macs / (float)peak_capacity) * 100.0f;
        out_stats->avg_mac_util_pct = (util > 100.0f) ? 100.0f : util;
    }

    if (self->total_cycles > 0) {
        out_stats->overall_ipc = (float)total_macs / (float)self->total_cycles;
    }

    return 0;
}

size_t npu_pmu_get_top_hotspots(const npu_pmu_t *pmu, npu_pmu_hotspot_t *out_hotspots, size_t max_count) {
    if (!pmu || !out_hotspots || max_count == 0) {
        return 0;
    }

    const npu_pmu_internal_t *self = (const npu_pmu_internal_t *)pmu;
    if (self->tracked_op_count == 0) {
        return 0;
    }

    /* Copy into temp array */
    npu_pmu_hotspot_t temp[NPU_PMU_MAX_TRACKED_OPS];
    size_t count = (self->tracked_op_count < NPU_PMU_MAX_TRACKED_OPS) ? self->tracked_op_count : NPU_PMU_MAX_TRACKED_OPS;

    uint64_t grand_total_ns = (self->total_latency_ns > 0) ? self->total_latency_ns : 1;

    for (size_t i = 0; i < count; i++) {
        const op_accumulator_t *acc = &self->op_stats[i];
        temp[i].op_id = acc->op_id;
        temp[i].op_type = acc->op_type;
        strncpy(temp[i].op_name, acc->op_name, sizeof(temp[i].op_name) - 1);
        temp[i].total_latency_ns = acc->total_latency_ns;
        temp[i].invocation_count = acc->count;
        temp[i].avg_latency_us = (acc->count > 0) ? (float)((double)acc->total_latency_ns / (double)acc->count / 1000.0) : 0.0f;
        temp[i].time_share_pct = (float)((double)acc->total_latency_ns / (double)grand_total_ns * 100.0);

        uint64_t cap = acc->total_cycles * (uint64_t)self->config.peak_macs_per_cycle;
        temp[i].avg_mac_util_pct = (cap > 0) ? ((float)acc->total_instructions / (float)cap * 100.0f) : 0.0f;

        uint64_t l2_tot = acc->total_l2_hits + acc->total_l2_misses;
        temp[i].avg_l2_miss_pct = (l2_tot > 0) ? ((float)acc->total_l2_misses / (float)l2_tot * 100.0f) : 0.0f;
    }

    /* Insertion sort descending by total_latency_ns */
    for (size_t i = 1; i < count; i++) {
        npu_pmu_hotspot_t key = temp[i];
        int j = (int)i - 1;
        while (j >= 0 && temp[j].total_latency_ns < key.total_latency_ns) {
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }

    size_t result_count = (count < max_count) ? count : max_count;
    for (size_t i = 0; i < result_count; i++) {
        out_hotspots[i] = temp[i];
    }

    return result_count;
}

int npu_pmu_sync_datahub(const npu_pmu_t *pmu, datahub_t *hub) {
    if (!pmu || !hub) {
        return -1;
    }

    npu_pmu_stats_t stats;
    if (npu_pmu_get_stats(pmu, &stats) != 0) {
        return -2;
    }

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_NPU_PMU_EXEC_CYCLES, (double)(stats.total_cycles & 0x7FFFFFFF), 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_PMU_CACHE_MISS_PCT, (double)stats.avg_l2_miss_pct, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_NPU_PMU_MAC_UTIL_PCT, (double)stats.avg_mac_util_pct, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}

void npu_pmu_reset(npu_pmu_t *pmu) {
    if (!pmu) {
        return;
    }

    npu_pmu_internal_t *self = (npu_pmu_internal_t *)pmu;
    npu_pmu_config_t cfg = self->config;
    memset(self, 0, sizeof(*self));
    self->config = cfg;
    self->next_token = 1;
}
