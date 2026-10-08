/**
 * @file edge_tmr_voter.c
 * @brief SIL-3 Triple Modular Redundancy (TMR) 2oo3 Voter & Hitless Failover Implementation (AEP-611)
 */

#include "edge_tmr_voter.h"
#include <string.h>
#include <math.h>

static inline float _tmr_abs(float x) {
    return (x >= 0.0f) ? x : -x;
}

static inline float _tmr_clamp(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

int edge_tmr_voter_init(tmr_voter_context_t *ctx, const tmr_voter_config_t *cfg) {
    if (!ctx) return -1;

    memset(ctx, 0, sizeof(*ctx));

    if (cfg) {
        ctx->config = *cfg;
    } else {
        ctx->config.tolerance_band = 0.05f;
        ctx->config.suspect_cycles_threshold = 3;
        ctx->config.failover_rate_limit = 50.0f;
        ctx->config.cycle_period_sec = 0.001f;
        ctx->config.fail_safe_value = 0.0f;
    }

    if (ctx->config.tolerance_band <= 0.0f) ctx->config.tolerance_band = 0.05f;
    if (ctx->config.suspect_cycles_threshold == 0) ctx->config.suspect_cycles_threshold = 3;
    if (ctx->config.failover_rate_limit <= 0.0f) ctx->config.failover_rate_limit = 50.0f;
    if (ctx->config.cycle_period_sec <= 0.0f) ctx->config.cycle_period_sec = 0.001f;

    for (int i = 0; i < EDGE_TMR_MAX_CHANNELS; i++) {
        ctx->node_health[i] = TMR_NODE_HEALTHY;
        ctx->node_outputs[i] = ctx->config.fail_safe_value;
        ctx->consecutive_faults[i] = 0;
    }

    ctx->healthy_count = EDGE_TMR_MAX_CHANNELS;
    ctx->active_master_idx = 0;
    ctx->last_target_output = ctx->config.fail_safe_value;
    ctx->current_target_output = ctx->config.fail_safe_value;
    ctx->smooth_output = ctx->config.fail_safe_value;

    return 0;
}

int edge_tmr_voter_update_channel(tmr_voter_context_t *ctx, uint8_t channel_idx, float value, tmr_node_health_t explicit_health) {
    if (!ctx || channel_idx >= EDGE_TMR_MAX_CHANNELS) return -1;

    ctx->node_outputs[channel_idx] = value;
    if (explicit_health != TMR_NODE_HEALTHY) {
        ctx->node_health[channel_idx] = explicit_health;
    }

    return 0;
}

int edge_tmr_voter_inject_fault(tmr_voter_context_t *ctx, uint8_t channel_idx, tmr_node_health_t fault_state) {
    if (!ctx || channel_idx >= EDGE_TMR_MAX_CHANNELS) return -1;
    ctx->node_health[channel_idx] = fault_state;
    if (fault_state != TMR_NODE_HEALTHY) {
        ctx->failover_in_progress = true;
    }
    return 0;
}

int edge_tmr_voter_reset_channel(tmr_voter_context_t *ctx, uint8_t channel_idx) {
    if (!ctx || channel_idx >= EDGE_TMR_MAX_CHANNELS) return -1;
    ctx->node_health[channel_idx] = TMR_NODE_HEALTHY;
    ctx->consecutive_faults[channel_idx] = 0;
    ctx->failover_in_progress = true;
    return 0;
}

int edge_tmr_voter_get_status(const tmr_voter_context_t *ctx, uint8_t *out_healthy_cnt, uint8_t *out_master_idx, bool *out_failover_active) {
    if (!ctx) return -1;
    if (out_healthy_cnt) *out_healthy_cnt = ctx->healthy_count;
    if (out_master_idx) *out_master_idx = ctx->active_master_idx;
    if (out_failover_active) *out_failover_active = ctx->failover_in_progress;
    return 0;
}

int edge_tmr_voter_step(tmr_voter_context_t *ctx, float *out_voted_value) {
    if (!ctx) return -1;

    ctx->sync_cycle_id++;

    /* 1. Identify healthy channels */
    uint8_t healthy_indices[EDGE_TMR_MAX_CHANNELS];
    uint8_t healthy_cnt = 0;
    uint8_t prev_healthy_cnt = ctx->healthy_count;

    for (uint8_t i = 0; i < EDGE_TMR_MAX_CHANNELS; i++) {
        if (ctx->node_health[i] == TMR_NODE_HEALTHY || ctx->node_health[i] == TMR_NODE_SUSPECT) {
            healthy_indices[healthy_cnt++] = i;
        }
    }
    ctx->healthy_count = healthy_cnt;

    float raw_voted = ctx->config.fail_safe_value;
    uint8_t prev_master = ctx->active_master_idx;

    if (healthy_cnt == 3) {
        float v0 = ctx->node_outputs[0];
        float v1 = ctx->node_outputs[1];
        float v2 = ctx->node_outputs[2];

        float d01 = _tmr_abs(v0 - v1);
        float d12 = _tmr_abs(v1 - v2);
        float d02 = _tmr_abs(v0 - v2);
        float tol = ctx->config.tolerance_band;

        /* Check for individual channel divergence */
        bool d0_fault = (d01 > tol) && (d02 > tol);
        bool d1_fault = (d01 > tol) && (d12 > tol);
        bool d2_fault = (d02 > tol) && (d12 > tol);

        if (d0_fault) {
            ctx->consecutive_faults[0]++;
            if (ctx->consecutive_faults[0] >= ctx->config.suspect_cycles_threshold) {
                ctx->node_health[0] = TMR_NODE_FAULTED;
            } else {
                ctx->node_health[0] = TMR_NODE_SUSPECT;
            }
        } else {
            ctx->consecutive_faults[0] = 0;
            ctx->node_health[0] = TMR_NODE_HEALTHY;
        }

        if (d1_fault) {
            ctx->consecutive_faults[1]++;
            if (ctx->consecutive_faults[1] >= ctx->config.suspect_cycles_threshold) {
                ctx->node_health[1] = TMR_NODE_FAULTED;
            } else {
                ctx->node_health[1] = TMR_NODE_SUSPECT;
            }
        } else {
            ctx->consecutive_faults[1] = 0;
            ctx->node_health[1] = TMR_NODE_HEALTHY;
        }

        if (d2_fault) {
            ctx->consecutive_faults[2]++;
            if (ctx->consecutive_faults[2] >= ctx->config.suspect_cycles_threshold) {
                ctx->node_health[2] = TMR_NODE_FAULTED;
            } else {
                ctx->node_health[2] = TMR_NODE_SUSPECT;
            }
        } else {
            ctx->consecutive_faults[2] = 0;
            ctx->node_health[2] = TMR_NODE_HEALTHY;
        }

        /* 3-way Mid-Value Selection (Median) */
        if ((v0 <= v1 && v1 <= v2) || (v2 <= v1 && v1 <= v0)) {
            raw_voted = v1;
            ctx->active_master_idx = 1;
        } else if ((v1 <= v0 && v0 <= v2) || (v2 <= v0 && v0 <= v1)) {
            raw_voted = v0;
            ctx->active_master_idx = 0;
        } else {
            raw_voted = v2;
            ctx->active_master_idx = 2;
        }
    } else if (healthy_cnt == 2) {
        uint8_t a = healthy_indices[0];
        uint8_t b = healthy_indices[1];
        float va = ctx->node_outputs[a];
        float vb = ctx->node_outputs[b];
        float tol = ctx->config.tolerance_band;

        if (_tmr_abs(va - vb) <= tol) {
            raw_voted = 0.5f * (va + vb);
            ctx->active_master_idx = a;
        } else {
            /* 1oo2D with discrepancy -> pick first healthy or hold */
            raw_voted = va;
            ctx->active_master_idx = a;
        }
    } else if (healthy_cnt == 1) {
        uint8_t a = healthy_indices[0];
        raw_voted = ctx->node_outputs[a];
        ctx->active_master_idx = a;
    } else {
        /* All channels lost -> fallback to fail-safe value */
        raw_voted = ctx->config.fail_safe_value;
    }

    /* Check if master changed or channel count reduced */
    if ((prev_master != ctx->active_master_idx || prev_healthy_cnt != healthy_cnt) && healthy_cnt < 3) {
        ctx->failover_in_progress = true;
        ctx->failover_event_count++;
    }

    ctx->last_target_output = ctx->current_target_output;
    ctx->current_target_output = raw_voted;

    if (ctx->sync_cycle_id == 1) {
        ctx->smooth_output = ctx->current_target_output;
        ctx->failover_in_progress = false;
    } else {
        /* 2. Bumpless Rate-Limiting for C1 Continuity */
        float delta = ctx->current_target_output - ctx->smooth_output;
        float max_step = ctx->config.failover_rate_limit * ctx->config.cycle_period_sec;

        if (max_step <= 0.0f) max_step = 1000.0f;

        if (_tmr_abs(delta) > max_step) {
            if (delta > 0.0f) {
                ctx->smooth_output += max_step;
            } else {
                ctx->smooth_output -= max_step;
            }
            ctx->failover_in_progress = true;
        } else {
            ctx->smooth_output = ctx->current_target_output;
            ctx->failover_in_progress = false;
        }
    }

    if (out_voted_value) {
        *out_voted_value = ctx->smooth_output;
    }

    return 0;
}
