#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"

static edge_driver_t mock_protocol = {
    .name = "MOCK_V3",
    .ctx_size = 16,
};

typedef struct {
    driver_manager_t *mgr;
} test_state_t;

static int setup(void **state) {
    test_state_t *ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create();
    driver_manager_register(ts->mgr, &mock_protocol);
    *state = ts;
    return 0;
}

static int teardown(void **state) {
    test_state_t *ts = *state;
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static void test_driver_instance_lifecycle(void **state) {
    test_state_t *ts = *state;
    edge_driver_ctx_t *ctx = driver_instance_create(ts->mgr, "MOCK_V3", 101);
    assert_non_null(ctx);
    assert_ptr_equal(ctx->driver, &mock_protocol);
    assert_non_null(ctx->priv);
    assert_int_equal(ctx->device_id, 101);

    driver_instance_destroy(ctx, "MOCK_V3");
}

static void test_driver_instance_invalid(void **state) {
    test_state_t *ts = *state;
    edge_driver_ctx_t *ctx = driver_instance_create(ts->mgr, "UNKNOWN_PROTO", 999);
    assert_null(ctx);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_driver_instance_lifecycle),
        cmocka_unit_test(test_driver_instance_invalid),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
