/**
 * @file test_ebpf_telemetry.c
 * @brief Unit tests for eBPF Kernel Latency Telemetry & Adaptive Circuit Breaker (AEP-612)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_ebpf_telemetry.h"

static void test_ebpf_telemetry_init_and_clean_state(void **state) {
    (void)state;
    edge_ebpf_telemetry_ctx_t ebpf_ctx;
    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 1234};
    assert_int_equal(edge_ebpf_telemetry_init(&ebpf_ctx, &cfg), 0);

    ebpf_latency_stats_t stats;
    int rc = edge_ebpf_telemetry_get_stats(&ebpf_ctx, &stats);
    assert_int_equal(0, rc);
    assert_int_equal(0, stats.sample_count);
    assert_int_equal(0, stats.max_jitter_us);
    assert_int_equal(EBPF_CB_CLOSED, ebpf_ctx.cb_state);
}

static void test_ebpf_telemetry_normal_latency_stream(void **state) {
    (void)state;
    edge_ebpf_telemetry_ctx_t ebpf_ctx;
    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 1234};
    edge_ebpf_telemetry_init(&ebpf_ctx, &cfg);

    /* Feed 100 healthy events (jitter < 15us, hardirq < 10us) */
    for (int i = 0; i < 100; i++) {
        uint32_t j = (uint32_t)(5 + (i % 10)); /* 5..14 us */
        edge_ebpf_telemetry_record_event(&ebpf_ctx, j, 8);
    }

    ebpf_latency_stats_t stats;
    edge_ebpf_telemetry_get_stats(&ebpf_ctx, &stats);
    assert_int_equal(100, stats.sample_count);
    assert_true(stats.avg_jitter_us <= 15);
    assert_true(stats.max_jitter_us <= 15);
    assert_int_equal(EBPF_CB_CLOSED, ebpf_ctx.cb_state);
    assert_false(ebpf_ctx.non_critical_tasks_paused);
}

static void test_ebpf_telemetry_circuit_breaker_trip_on_consecutive_spikes(void **state) {
    (void)state;
    edge_ebpf_telemetry_ctx_t ebpf_ctx;
    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 1234};
    edge_ebpf_telemetry_init(&ebpf_ctx, &cfg);

    /* 1 spike should cause WARNING */
    edge_ebpf_telemetry_record_event(&ebpf_ctx, 35, 10);
    assert_int_equal(EBPF_CB_WARNING, ebpf_ctx.cb_state);

    /* 2 more consecutive spikes exceeding trip threshold (65 us) */
    edge_ebpf_telemetry_record_event(&ebpf_ctx, 65, 10);
    edge_ebpf_telemetry_record_event(&ebpf_ctx, 70, 10);

    /* Now >= 3 consecutive spikes -> Tripped */
    assert_int_equal(EBPF_CB_TRIPPED, ebpf_ctx.cb_state);
    assert_true(ebpf_ctx.non_critical_tasks_paused);
    assert_true(ebpf_ctx.trip_count >= 1);
}

static void test_ebpf_telemetry_self_healing_recovery(void **state) {
    (void)state;
    edge_ebpf_telemetry_ctx_t ebpf_ctx;
    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 1234};
    edge_ebpf_telemetry_init(&ebpf_ctx, &cfg);

    /* Trip the circuit breaker */
    for (int i = 0; i < 4; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 80, 20);
    }
    assert_int_equal(EBPF_CB_TRIPPED, ebpf_ctx.cb_state);

    /* Feed 5 healthy events -> steps down to DEGRADED */
    for (int i = 0; i < 5; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 8, 5);
    }
    assert_int_equal(EBPF_CB_DEGRADED, ebpf_ctx.cb_state);

    /* Feed 5 more healthy events -> steps down to WARNING */
    for (int i = 0; i < 5; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 8, 5);
    }
    assert_int_equal(EBPF_CB_WARNING, ebpf_ctx.cb_state);
    assert_false(ebpf_ctx.non_critical_tasks_paused);

    /* Feed 5 more healthy events -> returns to CLOSED */
    for (int i = 0; i < 5; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 8, 5);
    }
    assert_int_equal(EBPF_CB_CLOSED, ebpf_ctx.cb_state);
}

static void test_ebpf_telemetry_histogram_and_percentiles(void **state) {
    (void)state;
    edge_ebpf_telemetry_ctx_t ebpf_ctx;
    ebpf_telemetry_config_t cfg = {.jitter_warn_us = 20,
                                   .jitter_trip_us = 50,
                                   .hardirq_trip_us = 100,
                                   .consecutive_spike_threshold = 3,
                                   .recovery_healthy_cycles = 5,
                                   .target_task_pid = 1234};
    edge_ebpf_telemetry_init(&ebpf_ctx, &cfg);

    /* Record distribution:
     * 90 samples at 8us (<10us bin)
     * 9 samples at 45us (<50us bin)
     * 1 sample at 250us (<300us bin)
     */
    for (int i = 0; i < 90; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 8, 5);
    }
    for (int i = 0; i < 9; i++) {
        edge_ebpf_telemetry_record_event(&ebpf_ctx, 45, 5);
    }
    edge_ebpf_telemetry_record_event(&ebpf_ctx, 250, 120);

    ebpf_latency_stats_t stats;
    edge_ebpf_telemetry_get_stats(&ebpf_ctx, &stats);
    assert_int_equal(100, stats.sample_count);
    assert_int_equal(250, stats.max_jitter_us);
    assert_int_equal(120, stats.hardirq_off_max_us);
    assert_true(stats.p90_jitter_us <= 10);
    assert_true(stats.p99_jitter_us >= 40);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ebpf_telemetry_init_and_clean_state),
        cmocka_unit_test(test_ebpf_telemetry_normal_latency_stream),
        cmocka_unit_test(test_ebpf_telemetry_circuit_breaker_trip_on_consecutive_spikes),
        cmocka_unit_test(test_ebpf_telemetry_self_healing_recovery),
        cmocka_unit_test(test_ebpf_telemetry_histogram_and_percentiles),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
