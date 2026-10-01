/**
 * @file test_cpu_affinity.c
 * @brief Unit tests for CPU Core Affinity Isolation, IRQ Tuning, and Memory Locking (AEP-716 /
 * Issue #247)
 */

#define _GNU_SOURCE
#include <cmocka.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "edge_cpu_affinity.h"

typedef struct {
    uint32_t pinned_count;
    uint32_t last_pinned_core;
    char last_task_name[32];
    uint32_t irq_migrated_count;
    bool memlock_notified;
    bool memlock_success;
} test_affinity_event_ctx_t;

static void test_on_core_pinned(void *ctx, pid_t tid, uint32_t core_id, const char *task_name) {
    (void)tid;
    test_affinity_event_ctx_t *evt = (test_affinity_event_ctx_t *)ctx;
    if (evt) {
        evt->pinned_count++;
        evt->last_pinned_core = core_id;
        if (task_name) {
            strncpy(evt->last_task_name, task_name, sizeof(evt->last_task_name) - 1);
        }
    }
}

static void test_on_irq_migrated(void *ctx, uint32_t irq_num, uint64_t prev_mask,
                                 uint64_t new_mask) {
    (void)irq_num;
    (void)prev_mask;
    (void)new_mask;
    test_affinity_event_ctx_t *evt = (test_affinity_event_ctx_t *)ctx;
    if (evt) {
        evt->irq_migrated_count++;
    }
}

static void test_on_memlock_applied(void *ctx, bool success, size_t prefaulted_bytes,
                                    int err_code) {
    (void)prefaulted_bytes;
    (void)err_code;
    test_affinity_event_ctx_t *evt = (test_affinity_event_ctx_t *)ctx;
    if (evt) {
        evt->memlock_notified = true;
        evt->memlock_success = success;
    }
}

static void test_cpu_topology_and_core_mask(void **state) {
    (void)state;
    edge_cpu_affinity_storage_t store;
    edge_cpu_affinity_config_t cfg = {
        .rt_core_mask = 0x0C,            // Cores 2, 3
        .housekeeping_core_mask = 0x03,  // Cores 0, 1
        .auto_isolate_irqs = false,
        .enable_mlockall = false,
    };

    edge_cpu_affinity_t *aff = edge_cpu_affinity_create_in(&store, &cfg, NULL);
    assert_non_null(aff);

    edge_cpu_topology_t topo;
    assert_int_equal(edge_cpu_affinity_get_topology(aff, &topo), 0);
    assert_true(topo.total_online_cores >= 1);
    assert_true(topo.available_core_mask > 0);
    assert_int_equal(topo.numa_nodes_count, 1);

    assert_int_equal(edge_cpu_affinity_destroy(aff), 0);
}

static void test_thread_pinning_and_affinity_mask(void **state) {
    (void)state;
    test_affinity_event_ctx_t evt_ctx = {0};
    edge_cpu_affinity_port_t port = {
        .on_core_pinned = test_on_core_pinned,
        .ctx = &evt_ctx,
    };

    edge_cpu_affinity_storage_t store;
    edge_cpu_affinity_config_t cfg = {
        .rt_core_mask = 0x02,
        .housekeeping_core_mask = 0x01,
        .auto_isolate_irqs = false,
        .enable_mlockall = false,
    };

    edge_cpu_affinity_t *aff = edge_cpu_affinity_create_in(&store, &cfg, &port);
    assert_non_null(aff);

    // Pin current thread to core 0
    assert_int_equal(edge_cpu_affinity_pin_current_thread(aff, 0, "iems_control_loop"), 0);
    assert_int_equal(evt_ctx.pinned_count, 1);
    assert_int_equal(evt_ctx.last_pinned_core, 0);
    assert_string_equal(evt_ctx.last_task_name, "iems_control_loop");

    // Pin current thread to out-of-range core
    assert_int_equal(edge_cpu_affinity_pin_current_thread(aff, 9999, "invalid_core"), -ERANGE);

    // Set core mask for current thread
    assert_int_equal(edge_cpu_affinity_set_mask_for_thread(aff, pthread_self(), 0x01), 0);

    edge_cpu_affinity_stats_t stats;
    assert_int_equal(edge_cpu_affinity_get_stats(aff, &stats), 0);
    assert_int_equal(stats.pinned_thread_count, 1);
    assert_int_equal(stats.rt_core_mask, 0x02);
    assert_int_equal(stats.housekeeping_core_mask, 0x01);

    assert_int_equal(edge_cpu_affinity_destroy(aff), 0);
}

static void test_memory_locking_and_stack_prefault(void **state) {
    (void)state;
    test_affinity_event_ctx_t evt_ctx = {0};
    edge_cpu_affinity_port_t port = {
        .on_memlock_applied = test_on_memlock_applied,
        .ctx = &evt_ctx,
    };

    edge_cpu_affinity_storage_t store;
    edge_cpu_affinity_config_t cfg = {
        .rt_core_mask = 0x01,
        .housekeeping_core_mask = 0x01,
        .auto_isolate_irqs = false,
        .enable_mlockall = true,
        .pre_fault_stack_size = 65536,  // 64KB
    };

    edge_cpu_affinity_t *aff = edge_cpu_affinity_create_in(&store, &cfg, &port);
    assert_non_null(aff);

    assert_true(evt_ctx.memlock_notified);

    edge_cpu_affinity_stats_t stats;
    assert_int_equal(edge_cpu_affinity_get_stats(aff, &stats), 0);
    if (evt_ctx.memlock_success) {
        assert_true(stats.memory_locked);
        assert_int_equal(stats.prefaulted_bytes, 65536);
    }

    assert_int_equal(edge_cpu_affinity_destroy(aff), 0);
}

static void test_irq_steering_and_diagnostics(void **state) {
    (void)state;
    test_affinity_event_ctx_t evt_ctx = {0};
    edge_cpu_affinity_port_t port = {
        .on_irq_migrated = test_on_irq_migrated,
        .ctx = &evt_ctx,
    };

    edge_cpu_affinity_storage_t store;
    edge_cpu_affinity_config_t cfg = {
        .rt_core_mask = 0x04,
        .housekeeping_core_mask = 0x01,
        .auto_isolate_irqs = true,
        .enable_mlockall = false,
    };

    edge_cpu_affinity_t *aff = edge_cpu_affinity_create_in(&store, &cfg, &port);
    assert_non_null(aff);

    uint32_t steered = 0;
    assert_int_equal(edge_cpu_affinity_steer_irqs(aff, 0x01, &steered), 0);

    edge_cpu_affinity_stats_t stats;
    assert_int_equal(edge_cpu_affinity_get_stats(aff, &stats), 0);

    assert_int_equal(edge_cpu_affinity_destroy(aff), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cpu_topology_and_core_mask),
        cmocka_unit_test(test_thread_pinning_and_affinity_mask),
        cmocka_unit_test(test_memory_locking_and_stack_prefault),
        cmocka_unit_test(test_irq_steering_and_diagnostics),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
