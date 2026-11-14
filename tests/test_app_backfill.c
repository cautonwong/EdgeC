/**
 * @file test_app_backfill.c
 * @brief Boundary, dedup, and live handoff for the backfill state machine.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edgeos/point_history.h"

typedef struct {
    edge_history_row_t rows[16];
    size_t count;
    /** Maximum rows returned per page, regardless of limit. */
    size_t chunk;
    size_t queries;
    uint64_t last_end_ts;
    int fail;
} fake_source_t;

typedef struct {
    edge_history_row_t rows[32];
    size_t count;
} fake_sink_t;

static edge_history_row_t _row(const char *point, int64_t ts, uint64_t seq) {
    edge_history_row_t row;
    memset(&row, 0, sizeof(row));
    snprintf(row.point, sizeof(row.point), "%s", point);
    row.source_ts = ts;
    row.seq = seq;
    row.value = (double)seq;
    row.quality = 192;
    snprintf(row.contract_version, sizeof(row.contract_version), "cv1");
    return row;
}

static int _src_query(void *ctx, const char *app_id, const char *contract_version,
                      const char *point, uint64_t end_ts, size_t limit, edge_history_row_t *out,
                      size_t *count, char *next_cursor, size_t cursor_cap) {
    fake_source_t *source = (fake_source_t *)ctx;
    (void)app_id;
    (void)contract_version;
    (void)point;
    if (source->fail) return -1;

    source->queries++;
    source->last_end_ts = end_ts;
    size_t start = next_cursor && next_cursor[0] ? (size_t)strtoul(next_cursor, NULL, 10) : 0;
    size_t max = limit < source->chunk ? limit : source->chunk;
    size_t copied = 0;
    size_t i = start;
    while (i < source->count && copied < max) {
        if ((uint64_t)source->rows[i].source_ts <= end_ts) out[copied++] = source->rows[i];
        i++;
    }
    if (i < source->count) {
        snprintf(next_cursor, cursor_cap, "%zu", i);
    } else if (next_cursor && cursor_cap) {
        next_cursor[0] = '\0';
    }
    *count = copied;
    return 0;
}

static int _sink_deliver(void *ctx, const edge_history_row_t *row) {
    fake_sink_t *sink = (fake_sink_t *)ctx;
    if (sink->count >= sizeof(sink->rows) / sizeof(sink->rows[0])) return -1;
    sink->rows[sink->count++] = *row;
    return 0;
}

static edge_backfill_result_t _live(app_backfill_t *bf, const char *point, int64_t ts,
                                    uint64_t seq) {
    edge_history_row_t row = _row(point, ts, seq);
    return edge_backfill_on_live(bf, &row);
}

static size_t _sink_key(const fake_sink_t *sink, const char *point, int64_t ts, uint64_t seq) {
    size_t found = 0;
    for (size_t i = 0; i < sink->count; i++) {
        if (sink->rows[i].source_ts == ts && sink->rows[i].seq == seq &&
            strcmp(sink->rows[i].point, point) == 0) {
            found++;
        }
    }
    return found;
}

static void test_boundary_dedup_and_live(void **state) {
    (void)state;
    fake_source_t source = {0};
    fake_sink_t sink = {0};
    app_backfill_t bf = {0};

    const struct {
        const char *point;
        int64_t ts;
        uint64_t seq;
    } history[] = {
        {"p1", 3000, 6}, {"p1", 2500, 2}, {"p1", 2500, 1}, {"p1", 2500, 2},
        {"p2", 2000, 3}, {"p1", 1500, 1}, {"p1", 1000, 1}, {"p1", 900, 1},
    };
    for (size_t i = 0; i < sizeof(history) / sizeof(history[0]); i++) {
        source.rows[source.count++] = _row(history[i].point, history[i].ts, history[i].seq);
    }
    source.chunk = 2;
    bf.source = (edge_history_source_t){.ctx = &source, .query = _src_query};
    bf.sink = (edge_backfill_sink_t){.ctx = &sink, .deliver = _sink_deliver};

    assert_int_equal(edge_backfill_begin(&bf, "app.demo", "cv1", 1000), EDGE_BACKFILL_OK);
    assert_int_equal(bf.state, EDGE_BACKFILL_SUBSCRIBE_BUFFER);

    // Live rows keep arriving while the history query has not run yet.
    assert_int_equal(_live(&bf, "p1", 1500, 1), EDGE_BACKFILL_OK);
    assert_int_equal(_live(&bf, "p1", 2500, 2), EDGE_BACKFILL_OK);
    assert_int_equal(_live(&bf, "p1", 3200, 1), EDGE_BACKFILL_OK);
    assert_int_equal(_live(&bf, "p1", 3200, 2), EDGE_BACKFILL_OK);
    assert_int_equal(_live(&bf, "p1", 3200, 2), EDGE_BACKFILL_OK);
    assert_int_equal(bf.buffered, 5);
    assert_int_equal(bf.delivered, 0);
    assert_int_equal(sink.count, 0);

    assert_int_equal(edge_backfill_step(&bf, 3000), EDGE_BACKFILL_OK);
    assert_int_equal(bf.state, EDGE_BACKFILL_LIVE);
    assert_int_equal(bf.delivered, 7);
    assert_int_equal(bf.duplicates_dropped, 4);
    assert_int_equal(bf.buffered, 0);
    assert_int_equal(sink.count, 7);
    assert_true(source.queries > 1);
    assert_int_equal(source.last_end_ts, 3000);

    // History window is (t0, begin_time], so the same-millisecond rows dedup by seq.
    assert_int_equal(_sink_key(&sink, "p1", 3000, 6), 1);
    assert_int_equal(_sink_key(&sink, "p1", 2500, 1), 1);
    assert_int_equal(_sink_key(&sink, "p1", 2500, 2), 1);
    assert_int_equal(_sink_key(&sink, "p2", 2000, 3), 1);
    assert_int_equal(_sink_key(&sink, "p1", 1500, 1), 1);
    assert_int_equal(_sink_key(&sink, "p1", 3200, 1), 1);
    assert_int_equal(_sink_key(&sink, "p1", 3200, 2), 1);
    assert_int_equal(_sink_key(&sink, "p1", 1000, 1), 0);
    assert_int_equal(_sink_key(&sink, "p1", 900, 1), 0);

    // No row is delivered under two keys.
    for (size_t i = 0; i < sink.count; i++) {
        for (size_t j = i + 1; j < sink.count; j++) {
            int same = sink.rows[i].source_ts == sink.rows[j].source_ts &&
                       sink.rows[i].seq == sink.rows[j].seq &&
                       strcmp(sink.rows[i].point, sink.rows[j].point) == 0;
            assert_false(same);
        }
    }

    // LIVE is reached once; later steps are no-ops.
    assert_int_equal(edge_backfill_step(&bf, 4000), EDGE_BACKFILL_OK);
    assert_int_equal(bf.state, EDGE_BACKFILL_LIVE);
    assert_int_equal(bf.delivered, 7);
    assert_int_equal(source.queries, 4);

    // Rows after the anchor are delivered immediately once live.
    assert_int_equal(_live(&bf, "p1", 5000, 1), EDGE_BACKFILL_OK);
    assert_int_equal(bf.delivered, 8);
    assert_int_equal(bf.buffered, 0);
    assert_int_equal(sink.count, 8);
    assert_int_equal(sink.rows[7].source_ts, 5000);
    assert_int_equal(sink.rows[7].seq, 1);

    edge_backfill_destroy(&bf);
}

static void test_source_failure_keeps_buffer_for_retry(void **state) {
    (void)state;
    fake_source_t source = {0};
    fake_sink_t sink = {0};
    app_backfill_t bf = {0};

    source.fail = 1;
    source.chunk = 4;
    bf.source = (edge_history_source_t){.ctx = &source, .query = _src_query};
    bf.sink = (edge_backfill_sink_t){.ctx = &sink, .deliver = _sink_deliver};

    assert_int_equal(edge_backfill_begin(&bf, "app.demo", "cv1", 0), EDGE_BACKFILL_OK);
    assert_int_equal(_live(&bf, "p1", 100, 1), EDGE_BACKFILL_OK);
    assert_int_equal(edge_backfill_step(&bf, 50), EDGE_BACKFILL_SOURCE);
    assert_int_equal(bf.state, EDGE_BACKFILL_HISTORY);
    assert_int_equal(bf.buffered, 1);
    assert_int_equal(bf.delivered, 0);

    source.fail = 0;
    assert_int_equal(edge_backfill_step(&bf, 60), EDGE_BACKFILL_OK);
    assert_int_equal(bf.state, EDGE_BACKFILL_LIVE);
    assert_int_equal(bf.delivered, 1);
    assert_int_equal(bf.duplicates_dropped, 0);
    assert_int_equal(bf.buffered, 0);
    assert_int_equal(sink.count, 1);
    assert_int_equal(sink.rows[0].source_ts, 100);

    edge_backfill_destroy(&bf);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_boundary_dedup_and_live),
        cmocka_unit_test(test_source_failure_keeps_buffer_for_retry),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
