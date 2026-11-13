/**
 * @file point_history.h
 * @brief Point history port and the subscribe-buffer-backfill-drain sequence.
 *
 * Backfill runs in four states: buffer live rows, read history down to T0,
 * drop rows the history already covered or that repeat a stable key, then go
 * live. The boundary is history covering (T0, begin_time] while buffered live
 * rows at or before begin_time are dropped as covered.
 */
#ifndef EDGEOS_POINT_HISTORY_H
#define EDGEOS_POINT_HISTORY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_HISTORY_POINT_SIZE 128
#define EDGE_HISTORY_CONTRACT_VERSION_SIZE 32
#define EDGE_HISTORY_APP_ID_SIZE 64
#define EDGE_HISTORY_CURSOR_SIZE 64
#define EDGE_BACKFILL_PAGE_SIZE 32u

typedef struct {
    char point[EDGE_HISTORY_POINT_SIZE];
    /** Source event time in epoch milliseconds. */
    int64_t source_ts;
    /** Monotonic sequence that orders rows sharing one source_ts. */
    uint64_t seq;
    double value;
    uint8_t quality;
    char contract_version[EDGE_HISTORY_CONTRACT_VERSION_SIZE];
} edge_history_row_t;

/**
 * History source port. One call returns at most limit rows with
 * source_ts <= end_ts, newest first (source_ts DESC, then seq DESC). point
 * NULL selects every point of the app and contract version.
 *
 * next_cursor is in/out: it carries the continuation from the previous call and
 * is rewritten empty once no rows remain. 0 on success, -1 on error.
 */
typedef struct {
    void *ctx;
    int (*query)(void *ctx, const char *app_id, const char *contract_version, const char *point,
                 uint64_t end_ts, size_t limit, edge_history_row_t *out, size_t *count,
                 char *next_cursor, size_t cursor_cap);
} edge_history_source_t;

/** Delivery sink port. 0 on success, -1 on error. */
typedef struct {
    void *ctx;
    int (*deliver)(void *ctx, const edge_history_row_t *row);
} edge_backfill_sink_t;

typedef enum {
    EDGE_BACKFILL_SUBSCRIBE_BUFFER = 0,
    EDGE_BACKFILL_HISTORY = 1,
    EDGE_BACKFILL_DEDUP_DRAIN = 2,
    EDGE_BACKFILL_LIVE = 3,
} edge_backfill_state_t;

typedef enum {
    EDGE_BACKFILL_OK = 0,
    EDGE_BACKFILL_INVALID,
    /** The history source failed. The step may be retried. */
    EDGE_BACKFILL_SOURCE,
    /** The delivery sink failed. The step may be retried, with at-least-once. */
    EDGE_BACKFILL_DELIVER,
    EDGE_BACKFILL_NOMEM,
} edge_backfill_result_t;

/** Dedup key, with app_id and contract_version fixed for one backfill. */
typedef struct {
    char point[EDGE_HISTORY_POINT_SIZE];
    int64_t source_ts;
    uint64_t seq;
} edge_history_key_t;

typedef struct app_backfill_s {
    /** Ports are set by the caller before edge_backfill_begin. */
    edge_history_source_t source;
    edge_backfill_sink_t sink;
    char app_id[EDGE_HISTORY_APP_ID_SIZE];
    char contract_version[EDGE_HISTORY_CONTRACT_VERSION_SIZE];
    int64_t t0_ms;
    /** History anchor, fixed at the first step's now. */
    int64_t begin_time_ms;
    edge_backfill_state_t state;
    uint64_t delivered;
    uint64_t duplicates_dropped;
    /** Rows currently held for the drain. */
    size_t buffered;
    /* internal */
    edge_history_row_t *buffer;
    size_t buffer_cap;
    edge_history_key_t *seen;
    size_t seen_count;
    size_t seen_cap;
} app_backfill_t;

/**
 * Start buffering. bf must be zero-initialized or destroyed. The history
 * anchor and the fixed ports are validated here.
 */
edge_backfill_result_t edge_backfill_begin(app_backfill_t *bf, const char *app_id,
                                           const char *contract_version, int64_t t0_ms);

/**
 * Accept one live row. Before LIVE it is buffered; in LIVE it is delivered
 * immediately. Live rows are not deduplicated in LIVE.
 */
edge_backfill_result_t edge_backfill_on_live(app_backfill_t *bf, const edge_history_row_t *row);

/**
 * Run the state machine to LIVE: query history to t0, deliver history rows,
 * drain buffered live rows above begin_time, then switch to LIVE. Reaching
 * LIVE takes one successful call; later calls are no-ops.
 */
edge_backfill_result_t edge_backfill_step(app_backfill_t *bf, int64_t now);

/** Release buffers. Safe on a zero-initialized struct. */
void edge_backfill_destroy(app_backfill_t *bf);

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_POINT_HISTORY_H */
