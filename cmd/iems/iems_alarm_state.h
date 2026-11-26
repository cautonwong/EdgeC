/**
 * @file iems_alarm_state.h
 * @brief P5-04a: real-time alarm table and its raise/ack/recover/suppress
 *        state machine, plus the injected vendor-tag mapping port.
 *
 * One row per (key, code, reason_code). "key" is the EdgeC tag the vendor tag
 * maps to; the vendor identity (class_type, vendor, model, tag) goes through
 * iems_alarm_tag_map_t and tests use a synthetic map. v3 keys both its info
 * and code lookups by reasonCode (alarm_init.c:284/294), so FaultCode1's 365
 * reason variants stay distinct rows instead of collapsing into one.
 *
 * State machine per row:
 *   absent --raise--> active --recover--> inactive (recovered_at set)
 *   inactive --raise--> active (acked cleared, recovered_at cleared)
 *   active --ack--> active (acked set); raise while active is a no-op
 *   suppressed is orthogonal: raise is ignored while it is set, and the flag
 *   survives recover and re-raise until resume. A suppressed row may exist
 *   before the alarm ever raises.
 *
 * Transitions are pure: no clock, no IO, no locking. The caller passes a
 * monotonic millisecond timestamp and serializes access. Return values:
 * 0 = transition applied, 1 = valid no-op (already in the target state),
 * -1 = invalid argument, table full, or unmapped tag.
 *
 * Capacity: IEMS_ALARM_STATE_MAX mirrors v3's MAX_ALARM_QUEUE_SIZE (1000,
 * alarm_define.h:41) with headroom. The zh_CN registry has 3821 definitions
 * and 3819 distinct (code, reasonCode) pairs, so raising every variant at
 * once still overflows and fails closed (return -1).
 */

#ifndef IEMS_ALARM_STATE_H
#define IEMS_ALARM_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_ALARM_STATE_MAX 1024u /* v3 MAX_ALARM_QUEUE_SIZE is 1000 */

/** @brief Live state of one (key, code, reason_code) alarm. */
typedef struct {
    uint32_t key;
    uint32_t code;
    int32_t reason_code;
    bool active;
    bool acked;
    bool suppressed;
    uint64_t recovered_at; /* caller's monotonic ms; 0 = never recovered */
} iems_alarm_state_t;

typedef struct {
    iems_alarm_state_t rows[IEMS_ALARM_STATE_MAX];
    uint32_t count;
} iems_alarm_table_t;

/** @brief Row for (key, code, reason_code), or NULL. */
const iems_alarm_state_t *iems_alarm_state_find(const iems_alarm_table_t *table, uint32_t key,
                                                uint32_t code, int32_t reason_code);

/** @brief Raise an alarm: create or reactivate; no-op while active or suppressed. */
int iems_alarm_state_raise(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                           int32_t reason_code);

/** @brief Acknowledge the active occurrence; no-op when inactive or already acked. */
int iems_alarm_state_ack(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                         int32_t reason_code);

/** @brief Recover the active occurrence; no-op unless active. */
int iems_alarm_state_recover(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                             int32_t reason_code, uint64_t now_ms);

/** @brief Inhibit raising; creates the row when absent. */
int iems_alarm_state_suppress(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                              int32_t reason_code);

/** @brief Clear suppression; no-op when absent or not suppressed. */
int iems_alarm_state_resume(iems_alarm_table_t *table, uint32_t key, uint32_t code,
                            int32_t reason_code);

/**
 * @brief Map a vendor (class_type, vendor, model, tag) to an EdgeC tag.
 *
 * Returns 0 on success with *edgec_tag set; any other return is unmapped.
 * The synthetic test map is the only implementation in this slice; the real
 * template mapping is UNCHECKED because B1 is missing.
 */
typedef struct {
    int (*map_tag)(void *ctx, const char *class_type, const char *vendor, const char *model,
                   const char *tag, uint32_t *edgec_tag);
    void *ctx;
} iems_alarm_tag_map_t;

/**
 * @brief Map through the port, then raise keyed by the mapped EdgeC tag and
 *        (code, reason_code).
 *
 * A mapping failure, a mapped tag of 0 (the unmapped sentinel) and NULL
 * arguments all return -1 without touching the table.
 */
int iems_alarm_state_raise_mapped(iems_alarm_table_t *table, const iems_alarm_tag_map_t *map,
                                  const char *class_type, const char *vendor, const char *model,
                                  const char *tag, uint32_t code, int32_t reason_code);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_ALARM_STATE_H */
