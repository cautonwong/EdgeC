/**
 * @file test_prio_booster.c
 * @brief Unit tests for Dynamic Priority Boosting and Annealing Decay Engine (AEP-718 / Issue #249)
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_prio_booster.h"

typedef struct {
    uint32_t boost_event_count;
    uint32_t last_boosted_task_id;
    int last_old_prio;
    int last_boosted_prio;
    uint32_t anneal_event_count;
    uint32_t last_annealed_task_id;
    int last_annealed_prio;
} test_booster_event_ctx_t;

static void test_on_priority_boosted(void *ctx, uint32_t task_id, int old_prio, int boosted_prio,
                                     const char *reason) {
    (void)reason;
    test_booster_event_ctx_t *evt = (test_booster_event_ctx_t *)ctx;
    if (evt) {
        evt->boost_event_count++;
        evt->last_boosted_task_id = task_id;
        evt->last_old_prio = old_prio;
        evt->last_boosted_prio = boosted_prio;
    }
}

static void test_on_priority_annealed(void *ctx, uint32_t task_id, int current_prio,
                                      int base_prio) {
    (void)base_prio;
    test_booster_event_ctx_t *evt = (test_booster_event_ctx_t *)ctx;
    if (evt) {
        evt->anneal_event_count++;
        evt->last_annealed_task_id = task_id;
        evt->last_annealed_prio = current_prio;
    }
}

static void test_prio_booster_lifecycle_and_registration(void **state) {
    (void)state;
    edge_prio_booster_storage_t store;
    edge_prio_booster_config_t cfg = {
        .tick_resolution_us = 1000,
        .default_boost_hold_us = 50000,
        .max_boosted_prio = 99,
        .anneal_curve = EDGE_ANNEAL_STEP,
    };

    edge_prio_booster_t *booster = edge_prio_booster_create_in(&store, &cfg, NULL);
    assert_non_null(booster);

    edge_boost_task_config_t t1 = {
        .task_id = 10,
        .name = "bess_thermal_watch",
        .base_priority = 20,
        .thread_handle = (pthread_t)0,
    };
    assert_int_equal(edge_prio_booster_register_task(booster, &t1), 0);

    // Duplicate ID should fail
    assert_int_equal(edge_prio_booster_register_task(booster, &t1), -EEXIST);

    int prio = 0;
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 10, &prio), 0);
    assert_int_equal(prio, 20);

    edge_prio_booster_stats_t stats;
    assert_int_equal(edge_prio_booster_get_stats(booster, &stats), 0);
    assert_int_equal(stats.registered_task_count, 1);
    assert_int_equal(stats.active_boosted_count, 0);

    assert_int_equal(edge_prio_booster_destroy(booster), 0);
}

static void test_prio_booster_instantaneous_boost_and_step_anneal(void **state) {
    (void)state;
    test_booster_event_ctx_t evt_ctx = {0};
    edge_prio_booster_port_t port = {
        .on_priority_boosted = test_on_priority_boosted,
        .on_priority_annealed = test_on_priority_annealed,
        .ctx = &evt_ctx,
    };

    edge_prio_booster_storage_t store;
    edge_prio_booster_config_t cfg = {
        .tick_resolution_us = 1000,
        .default_boost_hold_us = 20000,  // 20ms hold
        .max_boosted_prio = 99,
        .anneal_curve = EDGE_ANNEAL_STEP,
    };

    edge_prio_booster_t *booster = edge_prio_booster_create_in(&store, &cfg, &port);
    assert_non_null(booster);

    edge_boost_task_config_t t1 = {
        .task_id = 1,
        .name = "pcs_trip_handler",
        .base_priority = 15,
        .thread_handle = (pthread_t)0,
    };
    assert_int_equal(edge_prio_booster_register_task(booster, &t1), 0);

    // 1. Trigger boost to 99 with 20ms hold window
    assert_int_equal(edge_prio_booster_trigger_boost(booster, 1, 99, 20000, "pcs_overcurrent_trip"),
                     0);
    assert_int_equal(evt_ctx.boost_event_count, 1);
    assert_int_equal(evt_ctx.last_boosted_task_id, 1);
    assert_int_equal(evt_ctx.last_old_prio, 15);
    assert_int_equal(evt_ctx.last_boosted_prio, 99);

    int prio = 0;
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 1, &prio), 0);
    assert_int_equal(prio, 99);

    // 2. Step timer by 10ms (should still be held at 99)
    assert_int_equal(edge_prio_booster_tick(booster, 10000), 0);
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 1, &prio), 0);
    assert_int_equal(prio, 99);
    assert_int_equal(evt_ctx.anneal_event_count, 0);

    // 3. Step timer by another 15ms (total 25ms > 20ms, hold expired -> step annealed back to 15)
    assert_int_equal(edge_prio_booster_tick(booster, 15000), 0);
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 1, &prio), 0);
    assert_int_equal(prio, 15);
    assert_int_equal(evt_ctx.anneal_event_count, 1);
    assert_int_equal(evt_ctx.last_annealed_task_id, 1);
    assert_int_equal(evt_ctx.last_annealed_prio, 15);

    assert_int_equal(edge_prio_booster_destroy(booster), 0);
}

static void test_prio_booster_linear_decay_curve(void **state) {
    (void)state;
    test_booster_event_ctx_t evt_ctx = {0};
    edge_prio_booster_port_t port = {
        .on_priority_boosted = test_on_priority_boosted,
        .on_priority_annealed = test_on_priority_annealed,
        .ctx = &evt_ctx,
    };

    edge_prio_booster_storage_t store;
    edge_prio_booster_config_t cfg = {
        .tick_resolution_us = 1000,
        .default_boost_hold_us = 5000,  // 5ms hold
        .max_boosted_prio = 50,
        .anneal_curve = EDGE_ANNEAL_LINEAR,
    };

    edge_prio_booster_t *booster = edge_prio_booster_create_in(&store, &cfg, &port);
    assert_non_null(booster);

    edge_boost_task_config_t t1 = {
        .task_id = 5,
        .name = "vsg_frequency_droop",
        .base_priority = 30,
        .thread_handle = (pthread_t)0,
    };
    assert_int_equal(edge_prio_booster_register_task(booster, &t1), 0);

    // Boost to 50
    assert_int_equal(edge_prio_booster_trigger_boost(booster, 5, 50, 5000, "grid_frequency_dip"),
                     0);

    int prio = 0;
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 5, &prio), 0);
    assert_int_equal(prio, 50);

    // Step 5ms (hold expires)
    assert_int_equal(edge_prio_booster_tick(booster, 5000), 0);
    // Linear decay step (-5) -> 45
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 5, &prio), 0);
    assert_int_equal(prio, 45);

    // Step another tick -> decay (-5) -> 40
    assert_int_equal(edge_prio_booster_tick(booster, 1000), 0);
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 5, &prio), 0);
    assert_int_equal(prio, 40);

    // Step another tick -> decay (-5) -> 35
    assert_int_equal(edge_prio_booster_tick(booster, 1000), 0);
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 5, &prio), 0);
    assert_int_equal(prio, 35);

    // Step another tick -> decay to base 30 and complete anneal
    assert_int_equal(edge_prio_booster_tick(booster, 1000), 0);
    assert_int_equal(edge_prio_booster_get_task_priority(booster, 5, &prio), 0);
    assert_int_equal(prio, 30);
    assert_int_equal(evt_ctx.anneal_event_count, 1);

    assert_int_equal(edge_prio_booster_destroy(booster), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_prio_booster_lifecycle_and_registration),
        cmocka_unit_test(test_prio_booster_instantaneous_boost_and_step_anneal),
        cmocka_unit_test(test_prio_booster_linear_decay_curve),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
