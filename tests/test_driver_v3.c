#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"

extern edge_driver_t iec104_driver;

typedef struct {
    driver_manager_t *mgr;
} test_state_t;

static int setup(void **state) {
    test_state_t *ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create();
    driver_manager_register(ts->mgr, &iec104_driver);
    *state = ts;
    return 0;
}

static int teardown(void **state) {
    test_state_t *ts = *state;
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static void test_v3_session_state(void **state) {
    test_state_t *ts = *state;
    edge_driver_ctx_t *ctx = driver_instance_create(ts->mgr, "IEC104", 1);
    assert_non_null(ctx);
    driver_instance_destroy(ctx, "IEC104");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_v3_session_state),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
