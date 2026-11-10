/**
 * @file edge_tsn_engine.c
 * @brief IEEE 802.1Qbv TAS & IEEE 802.1AS TSN Deterministic Transport Engine Implementation
 */

#include "edge_tsn_engine.h"
#include <string.h>
#include <math.h>

int edge_tsn_engine_init(edge_tsn_engine_t *engine, const tsn_schedule_config_t *config) {
    if (!engine || !config) return -1;
    if (config->cycle_time_ns == 0 || config->entry_count == 0 || config->entry_count > TSN_MAX_GCL_ENTRIES) {
        return -2;
    }

    /* Verify cumulative intervals match cycle_time_ns */
    uint64_t sum = 0;
    for (uint32_t i = 0; i < config->entry_count; i++) {
        sum += config->entries[i].time_interval_ns;
    }
    if (sum != config->cycle_time_ns) {
        return -3; /* GCL sum mismatch */
    }

    memset(engine, 0, sizeof(edge_tsn_engine_t));
    memcpy(&engine->config, config, sizeof(tsn_schedule_config_t));
    engine->is_active = true;
    return 0;
}

int edge_tsn_sync_ptp(edge_tsn_engine_t *engine, uint64_t grandmaster_time_ns, uint64_t local_phc_time_ns) {
    if (!engine || !engine->is_active) return -1;

    int64_t current_offset = (int64_t)grandmaster_time_ns - (int64_t)local_phc_time_ns;
    if (engine->ptp.sync_count > 0 && engine->ptp.last_sync_time_ns > 0) {
        uint64_t dt = local_phc_time_ns - engine->ptp.last_sync_time_ns;
        if (dt > 0) {
            int64_t delta_offset = current_offset - engine->ptp.phc_offset_ns;
            engine->ptp.freq_drift_ppm = ((double)delta_offset / (double)dt) * 1e6;
        }
    }

    engine->ptp.last_offset_ns = engine->ptp.phc_offset_ns;
    engine->ptp.phc_offset_ns = current_offset;
    engine->ptp.last_sync_time_ns = local_phc_time_ns;
    engine->ptp.sync_count++;
    engine->ptp.is_synchronized = (llabs(current_offset) < 1000000LL); /* < 1ms = synchronized */

    engine->telemetry.current_phc_offset_ns = current_offset;
    return 0;
}

int edge_tsn_get_gate_state(const edge_tsn_engine_t *engine, uint64_t current_time_ns,
                            uint32_t *out_gate_mask, uint32_t *out_entry_idx) {
    if (!engine || !engine->is_active) return -1;

    uint64_t base_time = engine->config.base_time_ns;
    uint64_t cycle_time = engine->config.cycle_time_ns;

    uint64_t adjusted_time = current_time_ns + (uint64_t)engine->ptp.phc_offset_ns;
    uint64_t elapsed_in_cycle = 0;

    if (adjusted_time >= base_time) {
        elapsed_in_cycle = (adjusted_time - base_time) % cycle_time;
    } else {
        elapsed_in_cycle = 0;
    }

    uint64_t accum = 0;
    for (uint32_t i = 0; i < engine->config.entry_count; i++) {
        accum += engine->config.entries[i].time_interval_ns;
        if (elapsed_in_cycle < accum) {
            if (out_gate_mask) *out_gate_mask = engine->config.entries[i].gate_mask;
            if (out_entry_idx) *out_entry_idx = i;
            return 0;
        }
    }

    /* Fallback to last entry */
    if (out_gate_mask) *out_gate_mask = engine->config.entries[engine->config.entry_count - 1].gate_mask;
    if (out_entry_idx) *out_entry_idx = engine->config.entry_count - 1;
    return 0;
}

int edge_tsn_calculate_txtime(edge_tsn_engine_t *engine, uint8_t priority,
                              uint64_t req_send_time_ns, uint64_t *out_txtime_ns,
                              bool *out_gate_open) {
    if (!engine || !engine->is_active || priority >= TSN_MAX_PRIORITIES) return -1;

    uint32_t gate_mask = 0;
    uint32_t entry_idx = 0;
    edge_tsn_get_gate_state(engine, req_send_time_ns, &gate_mask, &entry_idx);

    bool is_open = (gate_mask & (1U << priority)) != 0;
    if (out_gate_open) *out_gate_open = is_open;

    if (is_open) {
        /* Gate is currently open, schedule at req_send_time_ns with small buffer margin (500ns) */
        if (out_txtime_ns) *out_txtime_ns = req_send_time_ns + 500;
        return 0;
    }

    /* If gate is closed, find next cycle/entry where gate opens for this priority */
    uint64_t cycle_time = engine->config.cycle_time_ns;
    uint64_t base_time = engine->config.base_time_ns;
    uint64_t adjusted_time = req_send_time_ns + (uint64_t)engine->ptp.phc_offset_ns;
    uint64_t cycle_start = base_time + ((adjusted_time >= base_time ? (adjusted_time - base_time) / cycle_time : 0) * cycle_time);
    uint64_t elapsed = (adjusted_time >= base_time) ? (adjusted_time - base_time) % cycle_time : 0;

    uint64_t next_open_offset = cycle_time; /* worst case next cycle */
    uint64_t accum = 0;

    /* Search remaining entries in current cycle */
    for (uint32_t i = 0; i < engine->config.entry_count; i++) {
        if (accum > elapsed && (engine->config.entries[i].gate_mask & (1U << priority))) {
            next_open_offset = accum;
            break;
        }
        accum += engine->config.entries[i].time_interval_ns;
    }

    /* If not found in rest of cycle, find first open entry in next cycle */
    if (next_open_offset == cycle_time) {
        accum = 0;
        for (uint32_t i = 0; i < engine->config.entry_count; i++) {
            if (engine->config.entries[i].gate_mask & (1U << priority)) {
                next_open_offset = cycle_time + accum;
                break;
            }
            accum += engine->config.entries[i].time_interval_ns;
        }
    }

    uint64_t target_txtime = cycle_start + next_open_offset + 500;
    if (out_txtime_ns) *out_txtime_ns = target_txtime;
    return 0;
}

int edge_tsn_record_tx_telemetry(edge_tsn_engine_t *engine, uint8_t priority,
                                 uint64_t sched_txtime_ns, uint64_t actual_tx_ns) {
    if (!engine || !engine->is_active || priority >= TSN_MAX_PRIORITIES) return -1;

    tsn_priority_stats_t *stats = &engine->telemetry.prio_stats[priority];
    stats->frames_sent++;
    engine->telemetry.total_scheduled_frames++;

    uint64_t jitter_ns = (actual_tx_ns >= sched_txtime_ns) ? (actual_tx_ns - sched_txtime_ns) : (sched_txtime_ns - actual_tx_ns);
    stats->total_jitter_ns += jitter_ns;
    if (jitter_ns > stats->max_jitter_ns) {
        stats->max_jitter_ns = jitter_ns;
    }

    /* Check window violation (jitter > 10us) */
    if (jitter_ns > 10000) {
        stats->window_violations++;
    }

    return 0;
}

void edge_tsn_get_telemetry(const edge_tsn_engine_t *engine, tsn_telemetry_t *out_telem) {
    if (engine && out_telem) {
        memcpy(out_telem, &engine->telemetry, sizeof(tsn_telemetry_t));
    }
}
