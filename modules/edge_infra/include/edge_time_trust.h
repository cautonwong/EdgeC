/**
 * @file edge_time_trust.h
 * @brief Wall-clock trust state (18): TRUSTED, DEGRADED, UNTRUSTED.
 *
 * Trust is raised only by an explicit mark from an admin action or a future
 * NTP/PTP reporter. The mark persists the highest wall time ever trusted, so a
 * wall clock that moves backwards is treated as untrusted even after a reboot.
 * Timers and timeouts elsewhere keep using monotonic clocks.
 */
#ifndef EDGE_TIME_TRUST_H
#define EDGE_TIME_TRUST_H

#include <stddef.h>
#include <stdint.h>

#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_TIME_TRUST_MAX_AGE_MS (24ull * 3600ull * 1000ull)
#define EDGE_TIME_TRUST_REASON_SIZE 160

typedef enum {
    EDGE_TIME_UNTRUSTED = 0,
    EDGE_TIME_DEGRADED = 1,
    EDGE_TIME_TRUSTED = 2,
} edge_time_state_t;

typedef struct {
    edge_time_state_t state;
    int has_rtc;
    uint64_t wall_ms;
    uint64_t monotonic_ms;
    uint64_t trusted_floor_ms;
    uint64_t last_sync_ms;
} edge_time_info_t;

const char *edge_time_state_name(edge_time_state_t state);

/** A platform RTC node is present. */
int edge_time_has_rtc(void);

uint64_t edge_time_now_wall_ms(void);
uint64_t edge_time_now_monotonic_ms(void);

/**
 * Pure classifier. A wall time below the trusted floor is UNTRUSTED; a recent
 * sync within max_age is TRUSTED; otherwise an RTC or a stale sync is DEGRADED.
 */
edge_time_state_t edge_time_classify(int has_rtc, uint64_t wall_ms, uint64_t floor_ms,
                                     uint64_t last_sync_ms, uint64_t max_age_ms);

/** Read the persisted floor and classify the given clock sample. */
int edge_time_trust_get(sqlite_svc_t *db, uint64_t wall_ms, uint64_t monotonic_ms,
                        edge_time_info_t *out, char *reason, size_t reason_size);

/** Mark the wall clock trusted; refuses a clock behind the persisted floor. */
int edge_time_trust_mark(sqlite_svc_t *db, uint64_t wall_ms, uint64_t monotonic_ms, char *reason,
                         size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_TIME_TRUST_H */
