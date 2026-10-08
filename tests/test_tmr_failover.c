/**
 * @file test_tmr_failover.c
 * @brief Unit tests for SIL-3 TMR 2oo3 Voter & Hitless Failover Engine (AEP-611)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_tmr_voter.h"

static void test_tmr_voter_all_healthy_consensus(void **state) {
    (void)state;
    tmr_voter_context_t voter_ctx;
    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 100.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};
    assert_int_equal(edge_tmr_voter_init(&voter_ctx, &cfg), 0);

    /* 3 channels have identical readings */
    edge_tmr_voter_update_channel(&voter_ctx, 0, 220.0f, TMR_NODE_HEALTHY);
    edge_tmr_voter_update_channel(&voter_ctx, 1, 220.0f, TMR_NODE_HEALTHY);
    edge_tmr_voter_update_channel(&voter_ctx, 2, 220.0f, TMR_NODE_HEALTHY);

    float voted = 0.0f;
    int rc = edge_tmr_voter_step(&voter_ctx, &voted);
    assert_int_equal(0, rc);
    assert_true(fabsf(voted - 220.0f) < 1.0f);

    uint8_t healthy_cnt = 0, master_idx = 0;
    bool failover = false;
    edge_tmr_voter_get_status(&voter_ctx, &healthy_cnt, &master_idx, &failover);
    assert_int_equal(3, healthy_cnt);
    assert_false(failover);
}

static void test_tmr_voter_mid_value_selection(void **state) {
    (void)state;
    tmr_voter_context_t voter_ctx;
    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 100.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};
    edge_tmr_voter_init(&voter_ctx, &cfg);

    /* 3 channels have slightly different values: 219.98, 220.02, 220.00 -> median is 220.00 */
    edge_tmr_voter_update_channel(&voter_ctx, 0, 219.98f, TMR_NODE_HEALTHY);
    edge_tmr_voter_update_channel(&voter_ctx, 1, 220.02f, TMR_NODE_HEALTHY);
    edge_tmr_voter_update_channel(&voter_ctx, 2, 220.00f, TMR_NODE_HEALTHY);

    float voted = 0.0f;
    for (int i = 0; i < 50; i++) {
        edge_tmr_voter_step(&voter_ctx, &voted);
    }
    assert_true(fabsf(voted - 220.00f) < 0.02f);
}

static void test_tmr_voter_single_channel_divergence_isolation(void **state) {
    (void)state;
    tmr_voter_context_t voter_ctx;
    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 100.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};
    edge_tmr_voter_init(&voter_ctx, &cfg);

    /* Channel 0 goes rogue (380V instead of 220V) */
    for (int cycle = 0; cycle < 5; cycle++) {
        edge_tmr_voter_update_channel(&voter_ctx, 0, 380.0f, TMR_NODE_HEALTHY);
        edge_tmr_voter_update_channel(&voter_ctx, 1, 220.0f, TMR_NODE_HEALTHY);
        edge_tmr_voter_update_channel(&voter_ctx, 2, 220.01f, TMR_NODE_HEALTHY);

        float voted = 0.0f;
        edge_tmr_voter_step(&voter_ctx, &voted);
    }

    /* Channel 0 should now be marked FAULTED */
    assert_int_equal(TMR_NODE_FAULTED, voter_ctx.node_health[0]);
    assert_int_equal(TMR_NODE_HEALTHY, voter_ctx.node_health[1]);
    assert_int_equal(TMR_NODE_HEALTHY, voter_ctx.node_health[2]);

    /* Healthy count should be 2 */
    uint8_t healthy_cnt = 0;
    edge_tmr_voter_get_status(&voter_ctx, &healthy_cnt, NULL, NULL);
    assert_int_equal(2, healthy_cnt);
}

static void test_tmr_voter_hitless_failover_bumpless(void **state) {
    (void)state;
    tmr_voter_context_t voter_ctx;
    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 100.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};
    edge_tmr_voter_init(&voter_ctx, &cfg);

    /* Stabilize at 100.0f */
    for (int i = 0; i < 100; i++) {
        edge_tmr_voter_update_channel(&voter_ctx, 0, 100.0f, TMR_NODE_HEALTHY);
        edge_tmr_voter_update_channel(&voter_ctx, 1, 100.0f, TMR_NODE_HEALTHY);
        edge_tmr_voter_update_channel(&voter_ctx, 2, 100.0f, TMR_NODE_HEALTHY);
        edge_tmr_voter_step(&voter_ctx, NULL);
    }

    /* Inject sudden master death and step change on remaining channels */
    edge_tmr_voter_inject_fault(&voter_ctx, 0, TMR_NODE_FAULTED);
    edge_tmr_voter_update_channel(&voter_ctx, 1, 150.0f, TMR_NODE_HEALTHY);
    edge_tmr_voter_update_channel(&voter_ctx, 2, 150.0f, TMR_NODE_HEALTHY);

    /* Single step must NOT jump 50.0f instantaneously (rate limit = 100/s * 0.001s = 0.1/step) */
    float first_step_voted = 0.0f;
    edge_tmr_voter_step(&voter_ctx, &first_step_voted);

    float delta = fabsf(first_step_voted - 100.0f);
    assert_true(delta <= 0.15f); /* Bump-less rate limit held */

    /* Run 600 steps to allow smooth ramp to 150.0f */
    float final_voted = 0.0f;
    for (int i = 0; i < 600; i++) {
        edge_tmr_voter_step(&voter_ctx, &final_voted);
    }
    assert_true(fabsf(final_voted - 150.0f) < 0.02f);
}

static void test_tmr_voter_all_faults_fail_safe(void **state) {
    (void)state;
    tmr_voter_context_t voter_ctx;
    tmr_voter_config_t cfg = {.tolerance_band = 0.05f,
                              .suspect_cycles_threshold = 3,
                              .failover_rate_limit = 100.0f,
                              .cycle_period_sec = 0.001f,
                              .fail_safe_value = 0.0f};
    edge_tmr_voter_init(&voter_ctx, &cfg);

    /* All 3 channels fault */
    edge_tmr_voter_inject_fault(&voter_ctx, 0, TMR_NODE_FAULTED);
    edge_tmr_voter_inject_fault(&voter_ctx, 1, TMR_NODE_FAULTED);
    edge_tmr_voter_inject_fault(&voter_ctx, 2, TMR_NODE_FAULTED);

    float voted = 100.0f;
    for (int i = 0; i < 200; i++) {
        edge_tmr_voter_step(&voter_ctx, &voted);
    }

    /* Reaches fail_safe_value = 0.0f */
    assert_true(fabsf(voted - 0.0f) < 0.02f);

    /* Reintegrate channel 0 */
    edge_tmr_voter_reset_channel(&voter_ctx, 0);
    edge_tmr_voter_update_channel(&voter_ctx, 0, 50.0f, TMR_NODE_HEALTHY);

    for (int i = 0; i < 600; i++) {
        edge_tmr_voter_step(&voter_ctx, &voted);
    }
    assert_true(fabsf(voted - 50.0f) < 0.02f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tmr_voter_all_healthy_consensus),
        cmocka_unit_test(test_tmr_voter_mid_value_selection),
        cmocka_unit_test(test_tmr_voter_single_channel_divergence_isolation),
        cmocka_unit_test(test_tmr_voter_hitless_failover_bumpless),
        cmocka_unit_test(test_tmr_voter_all_faults_fail_safe),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
