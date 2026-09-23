/**
 * @file test_resilient_buffer.c
 * @brief Unit tests for Resilient Telemetry Store-and-Forward Buffer Operator (with WAL
 * persistence)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "resilient_buffer.h"

static void test_resilient_buffer_init_null(void **state) {
    (void)state;
    assert_int_equal(resilient_buffer_init(NULL, 10), -1);
}

static void test_resilient_buffer_workflow(void **state) {
    (void)state;
    resilient_buffer_t buf;
    assert_int_equal(resilient_buffer_init(&buf, 100), 0);

    /* Simulate Store while online */
    assert_int_equal(resilient_buffer_push(&buf, 1001, 220.5, 1700000000ULL), 0);
    assert_int_equal(buf.count, 1);

    /* Simulate Network Outage */
    resilient_buffer_set_net_state(&buf, false);
    telemetry_record_t rec;
    assert_int_equal(resilient_buffer_pop(&buf, &rec),
                     1); /* Should be blocked/throttled during outage */

    /* Store 2 more during outage */
    assert_int_equal(resilient_buffer_push(&buf, 1002, 221.0, 1700000001ULL), 0);
    assert_int_equal(resilient_buffer_push(&buf, 1003, 221.5, 1700000002ULL), 0);
    assert_int_equal(buf.count, 3);

    /* Simulate Network Restored */
    resilient_buffer_set_net_state(&buf, true);
    assert_int_equal(resilient_buffer_pop(&buf, &rec), 0);
    assert_int_equal(rec.tag_id, 1001);
    assert_float_equal(rec.val, 220.5, 0.01);

    assert_int_equal(resilient_buffer_pop(&buf, &rec), 0);
    assert_int_equal(rec.tag_id, 1002);

    assert_int_equal(resilient_buffer_pop(&buf, &rec), 0);
    assert_int_equal(rec.tag_id, 1003);

    assert_int_equal(buf.count, 0);
    assert_int_equal(resilient_buffer_pop(&buf, &rec), 1); /* Empty */
}

static void test_resilient_buffer_wal_recovery(void **state) {
    (void)state;
    const char *wal_path = "test_resilient_recovery.wal";
    unlink(wal_path);

    resilient_buffer_t buf1, buf2;
    assert_int_equal(resilient_buffer_init(&buf1, 50), 0);
    assert_int_equal(resilient_buffer_push(&buf1, 3001, 12.5, 1800000000ULL), 0);
    assert_int_equal(resilient_buffer_push(&buf1, 3002, 13.0, 1800000001ULL), 0);

    /* Persist to WAL file */
    assert_int_equal(resilient_buffer_flush_to_file(&buf1, wal_path), 0);

    /* Simulate crash restart and recovery */
    assert_int_equal(resilient_buffer_init(&buf2, 50), 0);
    assert_int_equal(resilient_buffer_load_from_file(&buf2, wal_path), 0);
    assert_int_equal(buf2.count, 2);

    telemetry_record_t rec;
    assert_int_equal(resilient_buffer_pop(&buf2, &rec), 0);
    assert_int_equal(rec.tag_id, 3001);
    assert_float_equal(rec.val, 12.5, 0.01);

    assert_int_equal(resilient_buffer_pop(&buf2, &rec), 0);
    assert_int_equal(rec.tag_id, 3002);
    assert_float_equal(rec.val, 13.0, 0.01);

    unlink(wal_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_resilient_buffer_init_null),
        cmocka_unit_test(test_resilient_buffer_workflow),
        cmocka_unit_test(test_resilient_buffer_wal_recovery),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
