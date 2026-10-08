/**
 * @file edge_tmr_voter.h
 * @brief SIL-3 Triple Modular Redundancy (TMR) 2oo3 Voter & Hitless Failover Engine (AEP-611)
 *
 * Provides deterministic 2-out-of-3 mid-value selection, adaptive tolerance band voting,
 * health state machine (Healthy -> Suspect -> Faulted -> Isolated), and bumpless rate-limited
 * failover transitions ensuring C1 continuity in hard real-time industrial applications.
 *
 * Caller-allocated storage, zero dynamic memory allocation.
 */

#ifndef EDGE_TMR_VOTER_H
#define EDGE_TMR_VOTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_TMR_MAX_CHANNELS 3

typedef enum {
    TMR_NODE_HEALTHY  = 0,
    TMR_NODE_SUSPECT  = 1,
    TMR_NODE_FAULTED  = 2,
    TMR_NODE_ISOLATED = 3
} tmr_node_health_t;

typedef struct {
    float    tolerance_band;            /* Absolute tolerance band for divergence detection */
    uint32_t suspect_cycles_threshold;  /* Number of consecutive cycles of deviation to mark FAULTED */
    float    failover_rate_limit;       /* Maximum allowable rate of output change (units/sec) */
    float    cycle_period_sec;          /* Sampling/control cycle period in seconds */
    float    fail_safe_value;           /* Default value if all 3 channels fail */
} tmr_voter_config_t;

typedef struct {
    tmr_voter_config_t config;
    float              node_outputs[EDGE_TMR_MAX_CHANNELS];
    tmr_node_health_t  node_health[EDGE_TMR_MAX_CHANNELS];
    uint32_t           consecutive_faults[EDGE_TMR_MAX_CHANNELS];
    float              last_target_output;
    float              current_target_output;
    float              smooth_output;
    uint8_t            active_master_idx;
    uint8_t            healthy_count;
    bool               failover_in_progress;
    uint32_t           failover_event_count;
    uint64_t           sync_cycle_id;
    uint8_t            _reserved[16];
} tmr_voter_context_t;

/**
 * @brief Initialize TMR 2oo3 Voter with configuration.
 */
int edge_tmr_voter_init(tmr_voter_context_t *ctx, const tmr_voter_config_t *cfg);

/**
 * @brief Update sample reading and optional health flag for a specific channel.
 */
int edge_tmr_voter_update_channel(tmr_voter_context_t *ctx, uint8_t channel_idx, float value, tmr_node_health_t explicit_health);

/**
 * @brief Execute a voting cycle. Computes median/average, performs fault detection, and applies bumpless smoothing.
 */
int edge_tmr_voter_step(tmr_voter_context_t *ctx, float *out_voted_value);

/**
 * @brief Inject a fault into a specific channel for testing / simulation.
 */
int edge_tmr_voter_inject_fault(tmr_voter_context_t *ctx, uint8_t channel_idx, tmr_node_health_t fault_state);

/**
 * @brief Reintegrate / reset a faulted channel back to HEALTHY state.
 */
int edge_tmr_voter_reset_channel(tmr_voter_context_t *ctx, uint8_t channel_idx);

/**
 * @brief Query current voter operational status.
 */
int edge_tmr_voter_get_status(const tmr_voter_context_t *ctx, uint8_t *out_healthy_cnt, uint8_t *out_master_idx, bool *out_failover_active);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_TMR_VOTER_H */
