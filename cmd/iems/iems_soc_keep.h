/**
 * @file iems_soc_keep.h
 * @brief P5-03c: pure backup SOC-keep strategy decisions.
 *
 * Data shape: the caller injects resolved settings and one measurement per
 * tick; the evaluator returns the decision plus the state to feed back next
 * tick. It performs no IO, reads no clock and keeps no mutable globals, so the
 * same cfg/state/input always produce the same decision.
 *
 * State machine (at most one transition per evaluation):
 *
 *   from        condition                              to
 *   INACTIVE    soc < active_soc_pct                   PENDING_ON
 *   PENDING_ON  soc >= active_soc_pct                  INACTIVE (condition gone)
 *   PENDING_ON  soc < active, elapsed >= delay_on      ACTIVE
 *   ACTIVE      soc >= inactive_soc_pct                PENDING_OFF
 *   PENDING_OFF soc < inactive_soc_pct                 ACTIVE (condition gone)
 *   PENDING_OFF soc >= inactive, elapsed >= delay_off  INACTIVE
 *
 * delay_on_ms and delay_off_ms must be > 0; a zero delay would skip the
 * pending state and break the invariant, so it is BAD_CONFIG.
 *
 * v3 keeps get_sockeep_status() true through the exit delay
 * (strategy_sockeep.c:120-125: delay_trigger_bool_check still returns true
 * until it expires), so PENDING_OFF outputs the charge power and the caller
 * keeps charging. PENDING_ON outputs 0. Power is always <= 0 (正放负充).
 *
 * Reason mapping:
 *   DISABLED / UNSUPPORTED  not participating; state forced INACTIVE, outranks STALE
 *   SOC_BELOW_ACTIVE        first tick of an enter delay
 *   PENDING_ON              still waiting to enter
 *   SOC_ABOVE_INACTIVE      first tick of an exit delay, a completed exit, and
 *                           steady INACTIVE
 *   PENDING_OFF             still waiting to exit
 *   ACTIVE_CHARGE           ACTIVE, charging; also a cancelled exit delay
 *   BAD_CONFIG / STALE      no decision this tick; see iems_soc_keep_evaluate
 *
 * Thresholds: use iems_soc_keep_resolve_thresholds() to turn the raw v3
 * settings (active SOC, margin) into cfg thresholds. evaluate() trusts the
 * resolved values and rejects contradictory ones with BAD_CONFIG.
 */

#ifndef IEMS_SOC_KEEP_H
#define IEMS_SOC_KEEP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IEMS_SOC_KEEP_STATUS_INACTIVE = 0,
    IEMS_SOC_KEEP_STATUS_PENDING_ON,
    IEMS_SOC_KEEP_STATUS_ACTIVE,
    IEMS_SOC_KEEP_STATUS_PENDING_OFF
} iems_soc_keep_status_t;

typedef enum {
    IEMS_SOC_KEEP_REASON_DISABLED = 0,
    IEMS_SOC_KEEP_REASON_UNSUPPORTED,
    IEMS_SOC_KEEP_REASON_SOC_BELOW_ACTIVE,
    IEMS_SOC_KEEP_REASON_PENDING_ON,
    IEMS_SOC_KEEP_REASON_SOC_ABOVE_INACTIVE,
    IEMS_SOC_KEEP_REASON_PENDING_OFF,
    IEMS_SOC_KEEP_REASON_ACTIVE_CHARGE,
    IEMS_SOC_KEEP_REASON_BAD_CONFIG,
    IEMS_SOC_KEEP_REASON_STALE
} iems_soc_keep_reason_t;

/* v3 STRATAGY_TYPE values (jsonct.h:101-113) for the two strategies that
 * run SOC keep (strategy_sockeep.c:79-89). Any other value is UNSUPPORTED. */
typedef enum {
    IEMS_SOC_KEEP_STRATEGY_PERIOD_CTRL = 0,
    IEMS_SOC_KEEP_STRATEGY_PV_STORAGE = 3
} iems_soc_keep_strategy_t;

typedef struct {
    bool enable;
    double active_soc_pct;      /* enter threshold, 0..100 */
    double inactive_soc_pct;    /* exit threshold, active <= inactive <= 100 */
    double charge_power_kw;     /* positive magnitude; <= 5 uses max_charge_power_kw */
    double max_charge_power_kw; /* positive magnitude */
    uint32_t delay_on_ms;
    uint32_t delay_off_ms;
} iems_soc_keep_cfg_t;

typedef struct {
    iems_soc_keep_status_t status;
    uint64_t pending_since_ms;
    iems_soc_keep_status_t pending_target_status; /* derived: ACTIVE for PENDING_ON,
                                                     INACTIVE for PENDING_OFF, else status */
    uint64_t last_now_ms;
    bool first; /* set by iems_soc_keep_state_init until the first evaluation */
} iems_soc_keep_state_t;

typedef struct {
    uint64_t now_ms;
    double soc_pct;
    int strategy_type; /* iems_soc_keep_strategy_t */
    bool enabled;
} iems_soc_keep_input_t;

typedef struct {
    bool active; /* true while charging: ACTIVE or PENDING_OFF */
    double target_power_kw;
    iems_soc_keep_reason_t reason;
    iems_soc_keep_state_t next_state;
} iems_soc_keep_decision_t;

/* Returns a fresh INACTIVE state with first = true. */
iems_soc_keep_state_t iems_soc_keep_state_init(void);

/* v3 Strategy_SocKeep_cfg_update (strategy_sockeep.c:55-73): margin <= 0 uses
 * +5; inactive = active + margin; both clamp to 0..100. Returns false for
 * NaN/Inf, negative active, or a NULL output. */
bool iems_soc_keep_resolve_thresholds(double active_soc_pct, double margin_pct,
                                      double *out_active_pct, double *out_inactive_pct);

/* Pure evaluation; next_state is the caller's to carry. DISABLED/UNSUPPORTED
 * outrank STALE and reset to INACTIVE with 0 power, because no decision is
 * under way. BAD_CONFIG (NULL or non-finite inputs, active outside 0..100,
 * active > inactive, negative max_charge_power_kw, or a zero delay) resets to
 * INACTIVE with 0 power. STALE (participating, clock went backwards) leaves
 * the state untouched and keeps its power. */
iems_soc_keep_decision_t iems_soc_keep_evaluate(const iems_soc_keep_cfg_t *cfg,
                                                const iems_soc_keep_state_t *state,
                                                const iems_soc_keep_input_t *in);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_SOC_KEEP_H */
