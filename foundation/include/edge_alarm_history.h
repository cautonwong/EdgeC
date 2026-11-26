/**
 * @file edge_alarm_history.h
 * @brief Alarm history port, preserving the iems_v3 alarmRealTab semantics.
 *
 * One row is a raise or recovery event. rowid is the row identity; the table
 * has no primary key because duplicate raise times are legal in v3.
 */
#ifndef EDGE_ALARM_HISTORY_H
#define EDGE_ALARM_HISTORY_H

#include <stddef.h>
#include <stdint.h>

#include "edge_history_archive.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Column limits from v3: TEXT(64), TEXT(96), TEXT(64) plus the NUL byte. */
#define EDGE_ALARM_ENTITY_SIZE 65
#define EDGE_ALARM_TAGNAME_SIZE 97
#define EDGE_ALARM_ADDITION_SIZE 65

/** Query clamps: one page at most 1000 rows, first 10000 rows reachable. */
#define EDGE_ALARM_QUERY_PAGE_MAX 1000u
#define EDGE_ALARM_QUERY_OFFSET_MAX 10000u

#define EDGE_ALARM_STATUS_RECOVERY 0u
#define EDGE_ALARM_STATUS_ACTIVE 1u

typedef struct {
    /** v3 BIGINT UNSIGNED, epoch milliseconds. */
    int64_t raise_time_ms;
    /** 0 means not recovered; it is not a usable time. */
    int64_t recovery_time_ms;
    int32_t alarm_code;
    int32_t reason_code;
    char entity[EDGE_ALARM_ENTITY_SIZE];
    char tagname[EDGE_ALARM_TAGNAME_SIZE];
    char addition[EDGE_ALARM_ADDITION_SIZE];
    uint8_t status;
} edge_alarm_record_t;

typedef struct {
    void *ctx;
    /** Append one row. Over-long text returns INVALID, never truncates. */
    edge_history_result_t (*append)(void *ctx, const edge_alarm_record_t *record);
    /**
     * Match v3: alarm_code + reason_code + status=1 + raise_time, then set
     * status=0 and recovery_time. An unmatched key is not an error.
     */
    edge_history_result_t (*recover)(void *ctx, int32_t alarm_code, int32_t reason_code,
                                     int64_t raise_time_ms, int64_t recovery_time_ms);
    /**
     * active=1 lists status=1, active=0 lists status=0. Newest first
     * (raise_time DESC, rowid DESC). offset/limit are clamped; out_count is
     * the returned rows and total the matching rows before offset.
     */
    edge_history_result_t (*query)(void *ctx, int active, uint32_t offset, uint32_t limit,
                                   edge_alarm_record_t *out, uint32_t cap, uint32_t *out_count,
                                   uint32_t *total);
    edge_history_admin_port_t admin;
} edge_alarm_history_port_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALARM_HISTORY_H */
