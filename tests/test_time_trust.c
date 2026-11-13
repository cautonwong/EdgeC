/**
 * @file test_time_trust.c
 * @brief Clock trust classification and persisted trusted floor.
 */

#include <cmocka.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_time_trust.h"
#include "sqlite_service.h"

static void test_classification(void **state) {
    (void)state;
    const uint64_t max_age = 60000;
    assert_int_equal(edge_time_classify(0, 1000, 0, 0, max_age), EDGE_TIME_UNTRUSTED);
    assert_int_equal(edge_time_classify(1, 1000, 0, 0, max_age), EDGE_TIME_DEGRADED);
    assert_int_equal(edge_time_classify(0, 5000, 4000, 4000, max_age), EDGE_TIME_TRUSTED);
    assert_int_equal(edge_time_classify(0, 100000, 4000, 4000, max_age), EDGE_TIME_DEGRADED);
    assert_int_equal(edge_time_classify(1, 3000, 4000, 4000, max_age), EDGE_TIME_UNTRUSTED);
    assert_int_equal(edge_time_classify(0, 0, 0, 0, max_age), EDGE_TIME_UNTRUSTED);
}

static void test_mark_and_get(void **state) {
    (void)state;
    const char *db_path = "test_time_trust.db";
    unlink(db_path);
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);

    const uint64_t wall = 1700000000000ull;
    edge_time_info_t info;
    char reason[EDGE_TIME_TRUST_REASON_SIZE];
    assert_int_equal(edge_time_trust_get(db, wall, 10, &info, reason, sizeof(reason)), 0);
    assert_int_not_equal(info.state, EDGE_TIME_TRUSTED);
    assert_int_equal(info.trusted_floor_ms, 0);

    assert_int_equal(edge_time_trust_mark(db, wall, 10, reason, sizeof(reason)), 0);
    assert_int_equal(edge_time_trust_get(db, wall + 1000, 1010, &info, reason, sizeof(reason)),
                     0);
    assert_int_equal(info.state, EDGE_TIME_TRUSTED);
    assert_int_equal(info.trusted_floor_ms, wall);
    assert_int_equal(info.last_sync_ms, wall);

    assert_int_equal(edge_time_trust_get(db, wall - 1000, 1010, &info, reason, sizeof(reason)),
                     0);
    assert_int_equal(info.state, EDGE_TIME_UNTRUSTED);
    assert_int_not_equal(edge_time_trust_mark(db, wall - 1000, 1010, reason, sizeof(reason)), 0);
    assert_string_equal(reason, "wall clock is behind the trusted floor");

    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_classification),
        cmocka_unit_test(test_mark_and_get),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
