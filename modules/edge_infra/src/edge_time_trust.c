#include "edge_time_trust.h"

#include <sqlite3.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static int fail(char *reason, size_t size, const char *text) {
    if (reason && size) snprintf(reason, size, "%s", text);
    return -1;
}

const char *edge_time_state_name(edge_time_state_t state) {
    switch (state) {
    case EDGE_TIME_TRUSTED: return "trusted";
    case EDGE_TIME_DEGRADED: return "degraded";
    case EDGE_TIME_UNTRUSTED: return "untrusted";
    }
    return "untrusted";
}

int edge_time_has_rtc(void) {
    return access("/dev/rtc0", F_OK) == 0 || access("/dev/rtc", F_OK) == 0;
}

uint64_t edge_time_now_wall_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)(ts.tv_nsec / 1000000);
}

uint64_t edge_time_now_monotonic_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)(ts.tv_nsec / 1000000);
}

edge_time_state_t edge_time_classify(int has_rtc, uint64_t wall_ms, uint64_t floor_ms,
                                     uint64_t last_sync_ms, uint64_t max_age_ms) {
    if (wall_ms == 0) return EDGE_TIME_UNTRUSTED;
    if (floor_ms != 0 && wall_ms < floor_ms) return EDGE_TIME_UNTRUSTED;
    if (last_sync_ms != 0 && wall_ms >= last_sync_ms &&
        wall_ms - last_sync_ms <= max_age_ms) {
        return EDGE_TIME_TRUSTED;
    }
    if (has_rtc) return EDGE_TIME_DEGRADED;
    return last_sync_ms != 0 ? EDGE_TIME_DEGRADED : EDGE_TIME_UNTRUSTED;
}

static int load_uint(sqlite_svc_t *db, const char *key, uint64_t *out) {
    *out = 0;
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!handle) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(handle, "SELECT value FROM system_time WHERE key = ?;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) *out = (uint64_t)sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW || rc == SQLITE_DONE) ? 0 : -1;
}

static int save_uint(sqlite_svc_t *db, const char *key, uint64_t value) {
    sqlite3 *handle = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!handle) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(handle,
                           "INSERT OR REPLACE INTO system_time (key, value) VALUES (?, ?);", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)value);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

int edge_time_trust_get(sqlite_svc_t *db, uint64_t wall_ms, uint64_t monotonic_ms,
                        edge_time_info_t *out, char *reason, size_t reason_size) {
    if (!db || !out) return fail(reason, reason_size, "missing argument");
    uint64_t floor_ms = 0;
    uint64_t last_sync_ms = 0;
    if (load_uint(db, "trusted_floor_ms", &floor_ms) != 0 ||
        load_uint(db, "last_sync_ms", &last_sync_ms) != 0) {
        return fail(reason, reason_size, "time state is unreadable");
    }
    out->has_rtc = edge_time_has_rtc();
    out->wall_ms = wall_ms;
    out->monotonic_ms = monotonic_ms;
    out->trusted_floor_ms = floor_ms;
    out->last_sync_ms = last_sync_ms;
    out->state = edge_time_classify(out->has_rtc, wall_ms, floor_ms, last_sync_ms,
                                    EDGE_TIME_TRUST_MAX_AGE_MS);
    return 0;
}

int edge_time_trust_mark(sqlite_svc_t *db, uint64_t wall_ms, uint64_t monotonic_ms, char *reason,
                         size_t reason_size) {
    if (!db || wall_ms == 0) return fail(reason, reason_size, "invalid wall clock");
    uint64_t floor_ms = 0;
    if (load_uint(db, "trusted_floor_ms", &floor_ms) != 0) {
        return fail(reason, reason_size, "time state is unreadable");
    }
    if (floor_ms != 0 && wall_ms < floor_ms) {
        return fail(reason, reason_size, "wall clock is behind the trusted floor");
    }
    if (wall_ms > floor_ms && save_uint(db, "trusted_floor_ms", wall_ms) != 0) {
        return fail(reason, reason_size, "trusted floor write failed");
    }
    if (save_uint(db, "last_sync_ms", wall_ms) != 0 ||
        save_uint(db, "last_sync_mono_ms", monotonic_ms) != 0) {
        return fail(reason, reason_size, "sync time write failed");
    }
    if (reason && reason_size) reason[0] = '\0';
    return 0;
}
