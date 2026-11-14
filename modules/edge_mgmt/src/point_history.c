/**
 * @file point_history.c
 * @brief Subscribe-buffer, history backfill, dedup drain, then live.
 */

#include "edgeos/point_history.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool _valid_text(const char *text, size_t size) {
    if (!text) return false;
    size_t len = strlen(text);
    return len > 0 && len < size;
}

static int _buffer_push(app_backfill_t *bf, const edge_history_row_t *row) {
    if (bf->buffered == bf->buffer_cap) {
        size_t cap = bf->buffer_cap ? bf->buffer_cap * 2 : 8;
        edge_history_row_t *next = realloc(bf->buffer, cap * sizeof(*next));
        if (!next) return -1;
        bf->buffer = next;
        bf->buffer_cap = cap;
    }
    bf->buffer[bf->buffered++] = *row;
    return 0;
}

static bool _seen_has(const app_backfill_t *bf, const edge_history_row_t *row) {
    for (size_t i = 0; i < bf->seen_count; i++) {
        if (bf->seen[i].source_ts == row->source_ts && bf->seen[i].seq == row->seq &&
            strcmp(bf->seen[i].point, row->point) == 0) {
            return true;
        }
    }
    return false;
}

static int _seen_add(app_backfill_t *bf, const edge_history_row_t *row) {
    // shortcut: one key per delivered backfill row in RAM; move on-disk if one
    // backfill can exceed memory.
    if (bf->seen_count == bf->seen_cap) {
        size_t cap = bf->seen_cap ? bf->seen_cap * 2 : 16;
        edge_history_key_t *next = realloc(bf->seen, cap * sizeof(*next));
        if (!next) return -1;
        bf->seen = next;
        bf->seen_cap = cap;
    }
    edge_history_key_t *key = &bf->seen[bf->seen_count++];
    snprintf(key->point, sizeof(key->point), "%s", row->point);
    key->source_ts = row->source_ts;
    key->seq = row->seq;
    return 0;
}

// Key is marked after delivery so a failed sink retries the row instead of losing it.
static edge_backfill_result_t _deliver_row(app_backfill_t *bf, const edge_history_row_t *row) {
    if (_seen_has(bf, row)) {
        bf->duplicates_dropped++;
        return EDGE_BACKFILL_OK;
    }
    if (bf->sink.deliver(bf->sink.ctx, row) != 0) return EDGE_BACKFILL_DELIVER;
    bf->delivered++;
    if (_seen_add(bf, row) != 0) return EDGE_BACKFILL_NOMEM;
    return EDGE_BACKFILL_OK;
}

// History covers (t0, begin_time]; rows are newest first, so t0 ends the walk.
static edge_backfill_result_t _history(app_backfill_t *bf) {
    edge_history_row_t rows[EDGE_BACKFILL_PAGE_SIZE];
    char cursor[EDGE_HISTORY_CURSOR_SIZE] = {0};

    for (;;) {
        size_t count = 0;
        int rc = bf->source.query(bf->source.ctx, bf->app_id, bf->contract_version, NULL,
                                  (uint64_t)bf->begin_time_ms, EDGE_BACKFILL_PAGE_SIZE, rows,
                                  &count, cursor, sizeof(cursor));
        if (rc != 0) return EDGE_BACKFILL_SOURCE;

        for (size_t i = 0; i < count; i++) {
            if (rows[i].source_ts <= bf->t0_ms) return EDGE_BACKFILL_OK;
            edge_backfill_result_t result = _deliver_row(bf, &rows[i]);
            if (result != EDGE_BACKFILL_OK) return result;
        }
        if (count == 0 || cursor[0] == '\0') return EDGE_BACKFILL_OK;
    }
}

// Rows at or before begin_time are already covered by history.
static edge_backfill_result_t _drain(app_backfill_t *bf) {
    for (size_t i = 0; i < bf->buffered; i++) {
        const edge_history_row_t *row = &bf->buffer[i];
        if (row->source_ts <= bf->begin_time_ms) {
            bf->duplicates_dropped++;
            continue;
        }
        edge_backfill_result_t result = _deliver_row(bf, row);
        if (result != EDGE_BACKFILL_OK) return result;
    }
    bf->buffered = 0;
    return EDGE_BACKFILL_OK;
}

edge_backfill_result_t edge_backfill_begin(app_backfill_t *bf, const char *app_id,
                                           const char *contract_version, int64_t t0_ms) {
    if (!bf) return EDGE_BACKFILL_INVALID;
    edge_backfill_destroy(bf);
    if (!bf->source.query || !bf->sink.deliver) return EDGE_BACKFILL_INVALID;
    if (!_valid_text(app_id, EDGE_HISTORY_APP_ID_SIZE) ||
        !_valid_text(contract_version, EDGE_HISTORY_CONTRACT_VERSION_SIZE) || t0_ms < 0) {
        return EDGE_BACKFILL_INVALID;
    }

    snprintf(bf->app_id, sizeof(bf->app_id), "%s", app_id);
    snprintf(bf->contract_version, sizeof(bf->contract_version), "%s", contract_version);
    bf->t0_ms = t0_ms;
    bf->begin_time_ms = 0;
    bf->delivered = 0;
    bf->duplicates_dropped = 0;
    bf->state = EDGE_BACKFILL_SUBSCRIBE_BUFFER;
    return EDGE_BACKFILL_OK;
}

edge_backfill_result_t edge_backfill_on_live(app_backfill_t *bf, const edge_history_row_t *row) {
    if (!bf || !row) return EDGE_BACKFILL_INVALID;
    if (bf->state == EDGE_BACKFILL_LIVE) {
        if (!bf->sink.deliver) return EDGE_BACKFILL_INVALID;
        if (bf->sink.deliver(bf->sink.ctx, row) != 0) return EDGE_BACKFILL_DELIVER;
        bf->delivered++;
        return EDGE_BACKFILL_OK;
    }
    return _buffer_push(bf, row) == 0 ? EDGE_BACKFILL_OK : EDGE_BACKFILL_NOMEM;
}

edge_backfill_result_t edge_backfill_step(app_backfill_t *bf, int64_t now) {
    if (!bf || now < 0) return EDGE_BACKFILL_INVALID;
    if (bf->state == EDGE_BACKFILL_LIVE) return EDGE_BACKFILL_OK;
    if (!bf->source.query || !bf->sink.deliver) return EDGE_BACKFILL_INVALID;

    if (bf->state == EDGE_BACKFILL_SUBSCRIBE_BUFFER) {
        bf->begin_time_ms = now;
        bf->state = EDGE_BACKFILL_HISTORY;
    }
    if (bf->state == EDGE_BACKFILL_HISTORY) {
        edge_backfill_result_t result = _history(bf);
        if (result != EDGE_BACKFILL_OK) return result;
        bf->state = EDGE_BACKFILL_DEDUP_DRAIN;
    }
    if (bf->state == EDGE_BACKFILL_DEDUP_DRAIN) {
        edge_backfill_result_t result = _drain(bf);
        if (result != EDGE_BACKFILL_OK) return result;
        bf->state = EDGE_BACKFILL_LIVE;
    }
    return EDGE_BACKFILL_OK;
}

void edge_backfill_destroy(app_backfill_t *bf) {
    if (!bf) return;
    free(bf->buffer);
    free(bf->seen);
    bf->buffer = NULL;
    bf->buffer_cap = 0;
    bf->seen = NULL;
    bf->seen_count = 0;
    bf->seen_cap = 0;
    bf->buffered = 0;
}
