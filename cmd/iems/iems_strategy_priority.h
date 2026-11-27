/**
 * @file iems_strategy_priority.h
 * @brief P5-03d c1: pure key-precedence resolution for EMS and per-LC settings.
 *
 * Rules (P5-03d ticket, 业务规则):
 *   EMS:        current key > deprecated key > ABSENT
 *   per-LC:     lc_user_cfg override > lc_cfg base > ABSENT
 *
 * A NULL pointer means the key is absent. ABSENT is not the number 0: callers
 * must branch on the return code, never on value. Both keys present with
 * different values sets conflict; the higher-priority value wins and the two
 * values are never averaged or otherwise merged.
 *
 * deprecated_used answers "was a deprecated key supplied at all", not "which
 * key won"; source answers that. Both present therefore reports
 * source = CURRENT with deprecated_used = true, so migration warnings still
 * fire when a stale legacy key shadows the current one.
 *
 * The two resolvers are deliberately separate: an EMS Pmlmax/Pset and an LC
 * PowerLimit never travel together through one call, so they cannot be merged
 * by construction.
 *
 * Pure: no IO, no clock, no globals, no allocation.
 */

#ifndef IEMS_STRATEGY_PRIORITY_H
#define IEMS_STRATEGY_PRIORITY_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IEMS_PRIO_OK = 0,
    IEMS_PRIO_ABSENT,
    IEMS_PRIO_BAD_INPUT
} iems_prio_error_t;

typedef enum {
    IEMS_PRIO_SOURCE_ABSENT = 0,
    IEMS_PRIO_SOURCE_CURRENT,
    IEMS_PRIO_SOURCE_DEPRECATED,
    IEMS_PRIO_SOURCE_LC_OVERRIDE,
    IEMS_PRIO_SOURCE_LC_BASE
} iems_prio_source_t;

typedef struct {
    double value;              /* meaningful only when the return code is OK */
    iems_prio_source_t source; /* which key supplied value */
    bool deprecated_used;      /* a deprecated key was supplied (EMS only) */
    bool conflict;             /* both keys present with different values */
} iems_prio_result_t;

/* EMS key pair: Pmlmax/Pset current key vs its deprecated legacy key.
 * BAD_INPUT when out is NULL or any supplied value is not finite; out is then
 * reset to {0, ABSENT, false, false}. ABSENT when both keys are NULL.
 * conflict is an exact IEEE comparison; no epsilon, no averaging. +0.0 and
 * -0.0 compare equal under == (not bitwise), so they are not a conflict. */
iems_prio_error_t iems_strategy_priority_resolve_ems(const double *current,
                                                     const double *deprecated,
                                                     iems_prio_result_t *out);

/* Per-LC key pair: lc_user_cfg override vs lc_cfg base. deprecated_used is
 * always false. BAD_INPUT/ABSENT rules are the same as the EMS resolver. */
iems_prio_error_t iems_strategy_priority_resolve_lc(const double *override_value,
                                                    const double *base,
                                                    iems_prio_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_STRATEGY_PRIORITY_H */
