/**
 * @file test_federated_aggregator.c
 * @brief Unit tests for Edge Federated Learning Model Weight Delta Aggregator Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "federated_aggregator.h"

static void test_fed_aggregator_init_null(void **state) {
    (void)state;
    fed_aggregator_t agg;

    assert_int_equal(fed_aggregator_init(NULL, 10, NULL), -1);
    assert_int_equal(fed_aggregator_init(&agg, 0, NULL), -1);
}

static void test_fed_aggregator_fedavg_workflow(void **state) {
    (void)state;
    fed_aggregator_t agg;
    float init_weights[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    assert_int_equal(fed_aggregator_init(&agg, 4, init_weights), 0);
    assert_int_equal(agg.round_id, 1);

    float accum_buf[4] = {0.0f};

    /* Client 1: 100 samples, weights [2.0, 4.0, 6.0, 8.0] */
    float c1_weights[4] = {2.0f, 4.0f, 6.0f, 8.0f};
    assert_int_equal(fed_aggregator_accumulate(&agg, c1_weights, 100, accum_buf), 0);

    /* Client 2: 300 samples, weights [4.0, 8.0, 12.0, 16.0] */
    float c2_weights[4] = {4.0f, 8.0f, 12.0f, 16.0f};
    assert_int_equal(fed_aggregator_accumulate(&agg, c2_weights, 300, accum_buf), 0);

    assert_int_equal(agg.num_clients, 2);
    assert_int_equal(agg.total_samples, 400);

    /* Finalize FedAvg: Expected = (100*c1 + 300*c2)/400 = (c1 + 3*c2)/4 */
    assert_int_equal(fed_aggregator_finalize(&agg, accum_buf), 0);
    assert_int_equal(agg.round_id, 2);

    /* Expected: [ (2 + 12)/4 = 3.5, (4 + 24)/4 = 7.0, (6 + 36)/4 = 10.5, (8 + 48)/4 = 14.0 ] */
    assert_float_equal(agg.global_weights[0], 3.5f, 0.01f);
    assert_float_equal(agg.global_weights[1], 7.0f, 0.01f);
    assert_float_equal(agg.global_weights[2], 10.5f, 0.01f);
    assert_float_equal(agg.global_weights[3], 14.0f, 0.01f);

    fed_aggregator_free(&agg);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fed_aggregator_init_null),
        cmocka_unit_test(test_fed_aggregator_fedavg_workflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
