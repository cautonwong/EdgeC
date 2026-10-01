/**
 * @file test_pcp_mutex.c
 * @brief Unit tests for PREEMPT_RT Priority Ceiling Protocol (PCP) Mutex & Real-Time Lock Audit
 * (AEP-711 / Issue #244)
 */

#include <cmocka.h>
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "edge_pcp_mutex.h"

typedef struct {
    uint32_t overrun_count;
    uint32_t last_mutex_id;
    uint64_t last_hold_us;
    uint64_t last_expected_us;
} test_audit_ctx_t;

static void test_on_overrun(void *ctx, uint32_t mutex_id, uint64_t hold_duration_us,
                            uint64_t max_expected_us) {
    test_audit_ctx_t *audit = (test_audit_ctx_t *)ctx;
    if (audit) {
        audit->overrun_count++;
        audit->last_mutex_id = mutex_id;
        audit->last_hold_us = hold_duration_us;
        audit->last_expected_us = max_expected_us;
    }
}

static void test_pcp_mutex_basic_lifecycle(void **state) {
    (void)state;
    edge_pcp_mutex_storage_t store;
    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11001,
        .name = "test_pcp_vsg_lock",
        .protocol = EDGE_PCP_PROTOCOL_PROTECT,
        .prio_ceiling = 80,
        .max_expected_hold_us = 5000,
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, NULL);
    assert_non_null(m);

    // Basic lock / unlock
    int rc = edge_pcp_mutex_lock(m);
    assert_int_equal(rc, 0);

    edge_pcp_mutex_stats_t stats;
    rc = edge_pcp_mutex_get_stats(m, &stats);
    assert_int_equal(rc, 0);
    assert_true(stats.is_locked);
    assert_int_equal(stats.total_lock_count, 1);

    rc = edge_pcp_mutex_unlock(m);
    assert_int_equal(rc, 0);

    rc = edge_pcp_mutex_get_stats(m, &stats);
    assert_int_equal(rc, 0);
    assert_false(stats.is_locked);
    assert_int_equal(stats.total_lock_count, 1);
    assert_int_equal(stats.contention_count, 0);

    rc = edge_pcp_mutex_destroy(m);
    assert_int_equal(rc, 0);
}

static void test_pcp_mutex_trylock_and_timedlock(void **state) {
    (void)state;
    edge_pcp_mutex_storage_t store;
    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11002,
        .name = "test_pip_telemetry_lock",
        .protocol = EDGE_PCP_PROTOCOL_INHERIT,
        .prio_ceiling = 0,
        .max_expected_hold_us = 10000,
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, NULL);
    assert_non_null(m);

    int rc = edge_pcp_mutex_trylock(m);
    assert_int_equal(rc, 0);

    // Second trylock on non-recursive mutex should return EBUSY
    rc = edge_pcp_mutex_trylock(m);
    assert_int_equal(rc, EBUSY);

    rc = edge_pcp_mutex_unlock(m);
    assert_int_equal(rc, 0);

    // Timedlock
    rc = edge_pcp_mutex_timedlock(m, 50000);  // 50ms
    assert_int_equal(rc, 0);

    rc = edge_pcp_mutex_unlock(m);
    assert_int_equal(rc, 0);

    edge_pcp_mutex_stats_t stats;
    edge_pcp_mutex_get_stats(m, &stats);
    assert_int_equal(stats.total_lock_count, 2);
    assert_int_equal(stats.contention_count, 1);

    edge_pcp_mutex_destroy(m);
}

static void test_pcp_mutex_overrun_port(void **state) {
    (void)state;
    test_audit_ctx_t audit_ctx = {0};
    edge_pcp_mutex_port_t port = {
        .on_hold_overrun = test_on_overrun,
        .ctx = &audit_ctx,
    };

    edge_pcp_mutex_storage_t store;
    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11003,
        .name = "test_audit_fast_budget",
        .protocol = EDGE_PCP_PROTOCOL_NORMAL,
        .prio_ceiling = 0,
        .max_expected_hold_us = 1000,  // 1ms threshold
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, &port);
    assert_non_null(m);

    // Lock and sleep 3ms to exceed 1ms budget
    edge_pcp_mutex_lock(m);
    usleep(3000);
    edge_pcp_mutex_unlock(m);

    assert_int_equal(audit_ctx.overrun_count, 1);
    assert_int_equal(audit_ctx.last_mutex_id, 11003);
    assert_true(audit_ctx.last_hold_us >= 2000);

    edge_pcp_mutex_stats_t stats;
    edge_pcp_mutex_get_stats(m, &stats);
    assert_int_equal(stats.overrun_count, 1);
    assert_true(stats.max_hold_duration_ns >= 2000000ULL);

    edge_pcp_mutex_destroy(m);
}

typedef struct {
    edge_pcp_mutex_t *mutex;
    int iterations;
} thread_worker_args_t;

static void *contention_worker(void *arg) {
    thread_worker_args_t *w = (thread_worker_args_t *)arg;
    for (int i = 0; i < w->iterations; i++) {
        edge_pcp_mutex_lock(w->mutex);
        usleep(100);  // 100us critical section
        edge_pcp_mutex_unlock(w->mutex);
        usleep(50);
    }
    return NULL;
}

static void test_pcp_mutex_multithread_contention(void **state) {
    (void)state;
    edge_pcp_mutex_storage_t store;
    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11004,
        .name = "test_contention_pool",
        .protocol = EDGE_PCP_PROTOCOL_PROTECT,
        .prio_ceiling = 70,
        .max_expected_hold_us = 50000,
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, NULL);
    assert_non_null(m);

    pthread_t t1, t2, t3;
    thread_worker_args_t args = {
        .mutex = m,
        .iterations = 20,
    };

    pthread_create(&t1, NULL, contention_worker, &args);
    pthread_create(&t2, NULL, contention_worker, &args);
    pthread_create(&t3, NULL, contention_worker, &args);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    edge_pcp_mutex_stats_t stats;
    edge_pcp_mutex_get_stats(m, &stats);

    assert_int_equal(stats.total_lock_count, 60);
    assert_false(stats.is_locked);
    // Across 3 concurrent threads competing for 60 iterations, contention must occur
    assert_true(stats.contention_count > 0);
    assert_true(stats.inversions_prevented > 0);

    edge_pcp_mutex_destroy(m);
}

static void test_pcp_mutex_prioceiling_adjust(void **state) {
    (void)state;
    edge_pcp_mutex_storage_t store;
    edge_pcp_mutex_config_t cfg = {
        .mutex_id = 11005,
        .name = "test_dyn_ceiling",
        .protocol = EDGE_PCP_PROTOCOL_PROTECT,
        .prio_ceiling = 50,
        .max_expected_hold_us = 10000,
        .recursive = false,
    };

    edge_pcp_mutex_t *m = edge_pcp_mutex_create_in(&store, &cfg, NULL);
    assert_non_null(m);

    int old_ceiling = 0;
    // Attempt adjusting priority ceiling
    int rc = edge_pcp_mutex_set_prioceiling(m, 85, &old_ceiling);
    if (rc == 0) {
        edge_pcp_mutex_stats_t stats;
        edge_pcp_mutex_get_stats(m, &stats);
        assert_int_equal(stats.prio_ceiling, 85);
    }

    edge_pcp_mutex_destroy(m);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pcp_mutex_basic_lifecycle),
        cmocka_unit_test(test_pcp_mutex_trylock_and_timedlock),
        cmocka_unit_test(test_pcp_mutex_overrun_port),
        cmocka_unit_test(test_pcp_mutex_multithread_contention),
        cmocka_unit_test(test_pcp_mutex_prioceiling_adjust),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
