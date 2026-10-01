/**
 * @file test_lockfree_ring.c
 * @brief Unit tests for Lock-Free Zero-Copy RingBuffer Channel (AEP-717 / Issue #248)
 */

#include <cmocka.h>
#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_lockfree_ring.h"

typedef struct {
    uint32_t watermark_trigger_count;
    uint32_t last_watermark_depth;
    uint32_t overflow_trigger_count;
    uint64_t last_drop_count;
} test_ring_event_ctx_t;

typedef struct {
    uint32_t seq;
    uint64_t timestamp_ns;
    double value;
} test_payload_t;

static void test_on_watermark_high(void *ctx, uint32_t current_depth, uint32_t capacity) {
    (void)capacity;
    test_ring_event_ctx_t *evt = (test_ring_event_ctx_t *)ctx;
    if (evt) {
        evt->watermark_trigger_count++;
        evt->last_watermark_depth = current_depth;
    }
}

static void test_on_overflow(void *ctx, uint64_t total_drops, const char *reason) {
    (void)reason;
    test_ring_event_ctx_t *evt = (test_ring_event_ctx_t *)ctx;
    if (evt) {
        evt->overflow_trigger_count++;
        evt->last_drop_count = total_drops;
    }
}

static void test_lockfree_ring_spsc_basic_and_zerocopy(void **state) {
    (void)state;
    test_ring_event_ctx_t evt_ctx = {0};
    edge_lockfree_ring_port_t port = {
        .on_watermark_high = test_on_watermark_high,
        .on_overflow = test_on_overflow,
        .ctx = &evt_ctx,
    };

    edge_lockfree_ring_storage_t store;
    edge_lockfree_ring_config_t cfg = {
        .mode = EDGE_RING_MODE_SPSC,
        .overflow_policy = EDGE_RING_OVERFLOW_DROP,
        .capacity = 16,
        .elem_size_bytes = (uint32_t)sizeof(test_payload_t),
        .high_watermark_percent = 75,
    };

    edge_lockfree_ring_t *ring = edge_lockfree_ring_create_in(&store, &cfg, &port);
    assert_non_null(ring);
    assert_int_equal(edge_lockfree_ring_capacity(ring), 16);
    assert_int_equal(edge_lockfree_ring_depth(ring), 0);

    // 1. Basic Enqueue & Dequeue
    test_payload_t p1 = {.seq = 101, .timestamp_ns = 1000000ULL, .value = 380.0};
    assert_int_equal(edge_lockfree_ring_enqueue(ring, &p1), 0);
    assert_int_equal(edge_lockfree_ring_depth(ring), 1);

    test_payload_t out1 = {0};
    assert_int_equal(edge_lockfree_ring_dequeue(ring, &out1), 0);
    assert_int_equal(out1.seq, 101);
    assert_float_equal(out1.value, 380.0, 0.001);
    assert_int_equal(edge_lockfree_ring_depth(ring), 0);

    // Dequeue empty
    assert_int_equal(edge_lockfree_ring_dequeue(ring, &out1), -ENOENT);

    // 2. Zero-Copy Reserve & Commit
    void *slot = NULL;
    uint64_t ticket = 0;
    assert_int_equal(edge_lockfree_ring_reserve(ring, &slot, &ticket), 0);
    assert_non_null(slot);

    test_payload_t *p_slot = (test_payload_t *)slot;
    p_slot->seq = 202;
    p_slot->timestamp_ns = 2000000ULL;
    p_slot->value = 220.5;

    assert_int_equal(edge_lockfree_ring_commit(ring, ticket), 0);
    assert_int_equal(edge_lockfree_ring_depth(ring), 1);

    // 3. Zero-Copy Peek & Advance
    const void *peek_slot = NULL;
    assert_int_equal(edge_lockfree_ring_peek(ring, &peek_slot), 0);
    assert_non_null(peek_slot);
    const test_payload_t *p_peek = (const test_payload_t *)peek_slot;
    assert_int_equal(p_peek->seq, 202);
    assert_float_equal(p_peek->value, 220.5, 0.001);

    assert_int_equal(edge_lockfree_ring_advance(ring), 0);
    assert_int_equal(edge_lockfree_ring_depth(ring), 0);

    assert_int_equal(edge_lockfree_ring_destroy(ring), 0);
}

typedef struct {
    edge_lockfree_ring_t *ring;
    uint32_t count_per_thread;
    uint32_t thread_idx;
} thread_test_arg_t;

static void *mpmc_producer_worker(void *arg) {
    thread_test_arg_t *targ = (thread_test_arg_t *)arg;
    for (uint32_t i = 0; i < targ->count_per_thread; i++) {
        test_payload_t p = {
            .seq = (targ->thread_idx * 10000) + i,
            .timestamp_ns = (uint64_t)i,
            .value = (double)i,
        };
        while (edge_lockfree_ring_enqueue(targ->ring, &p) != 0) {
            sched_yield();
        }
    }
    return NULL;
}

static void *mpmc_consumer_worker(void *arg) {
    thread_test_arg_t *targ = (thread_test_arg_t *)arg;
    for (uint32_t i = 0; i < targ->count_per_thread; i++) {
        test_payload_t p = {0};
        while (edge_lockfree_ring_dequeue(targ->ring, &p) != 0) {
            sched_yield();
        }
    }
    return NULL;
}

static void test_lockfree_ring_mpmc_concurrency(void **state) {
    (void)state;
    edge_lockfree_ring_storage_t store;
    edge_lockfree_ring_config_t cfg = {
        .mode = EDGE_RING_MODE_MPMC,
        .overflow_policy = EDGE_RING_OVERFLOW_DROP,
        .capacity = 64,
        .elem_size_bytes = (uint32_t)sizeof(test_payload_t),
        .high_watermark_percent = 80,
    };

    edge_lockfree_ring_t *ring = edge_lockfree_ring_create_in(&store, &cfg, NULL);
    assert_non_null(ring);

    const uint32_t NUM_THREADS = 4;
    const uint32_t ITEMS_PER_THREAD = 1000;
    pthread_t producers[4];
    pthread_t consumers[4];
    thread_test_arg_t prod_args[4];
    thread_test_arg_t cons_args[4];

    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        prod_args[i] = (thread_test_arg_t){
            .ring = ring, .count_per_thread = ITEMS_PER_THREAD, .thread_idx = i + 1};
        cons_args[i] = (thread_test_arg_t){
            .ring = ring, .count_per_thread = ITEMS_PER_THREAD, .thread_idx = i + 1};
    }

    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        pthread_create(&consumers[i], NULL, mpmc_consumer_worker, &cons_args[i]);
        pthread_create(&producers[i], NULL, mpmc_producer_worker, &prod_args[i]);
    }

    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    edge_lockfree_ring_stats_t stats;
    assert_int_equal(edge_lockfree_ring_get_stats(ring, &stats), 0);
    assert_int_equal(stats.total_enqueued, (uint64_t)(NUM_THREADS * ITEMS_PER_THREAD));
    assert_int_equal(stats.total_dequeued, (uint64_t)(NUM_THREADS * ITEMS_PER_THREAD));
    assert_int_equal(edge_lockfree_ring_depth(ring), 0);

    assert_int_equal(edge_lockfree_ring_destroy(ring), 0);
}

static void test_lockfree_ring_watermark_and_overflow(void **state) {
    (void)state;
    test_ring_event_ctx_t evt_ctx = {0};
    edge_lockfree_ring_port_t port = {
        .on_watermark_high = test_on_watermark_high,
        .on_overflow = test_on_overflow,
        .ctx = &evt_ctx,
    };

    edge_lockfree_ring_storage_t store;
    edge_lockfree_ring_config_t cfg = {
        .mode = EDGE_RING_MODE_SPSC,
        .overflow_policy = EDGE_RING_OVERFLOW_DROP,
        .capacity = 8,
        .elem_size_bytes = (uint32_t)sizeof(test_payload_t),
        .high_watermark_percent = 50,  // Triggers at 4 items
    };

    edge_lockfree_ring_t *ring = edge_lockfree_ring_create_in(&store, &cfg, &port);
    assert_non_null(ring);

    for (uint32_t i = 0; i < 8; i++) {
        test_payload_t p = {.seq = i, .timestamp_ns = i * 100, .value = (double)i};
        assert_int_equal(edge_lockfree_ring_enqueue(ring, &p), 0);
    }

    assert_true(evt_ctx.watermark_trigger_count > 0);

    // 9th element should fail with -ENOSPC
    test_payload_t overflow_p = {.seq = 999, .timestamp_ns = 0, .value = 0.0};
    assert_int_equal(edge_lockfree_ring_enqueue(ring, &overflow_p), -ENOSPC);
    assert_int_equal(evt_ctx.overflow_trigger_count, 1);
    assert_int_equal(evt_ctx.last_drop_count, 1);

    assert_int_equal(edge_lockfree_ring_destroy(ring), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lockfree_ring_spsc_basic_and_zerocopy),
        cmocka_unit_test(test_lockfree_ring_mpmc_concurrency),
        cmocka_unit_test(test_lockfree_ring_watermark_and_overflow),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
