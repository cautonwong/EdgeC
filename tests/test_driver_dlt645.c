#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "net_buf.h"

extern edge_driver_t dlt645_driver;

typedef struct {
    driver_manager_t* mgr;
    edge_driver_ctx_t* ctx;
    net_buf_pool_t pool;
} test_state_t;

static int group_setup(void** state) {
    test_state_t* ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create();
    driver_manager_register(ts->mgr, &dlt645_driver);
    net_buf_pool_init(&ts->pool, 16, 512, 0, NULL, NULL);
    *state = ts;
    return 0;
}

static int group_teardown(void** state) {
    test_state_t* ts = *state;
    net_buf_pool_destroy(&ts->pool);
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static int test_setup(void** state) {
    test_state_t* ts = *state;
    ts->ctx = driver_instance_create(ts->mgr, "DLT645", 123456789);
    return 0;
}

static int test_teardown(void** state) {
    test_state_t* ts = *state;
    driver_instance_destroy(ts->ctx, "DLT645");
    return 0;
}

static void test_dlt645_v3_cycle(void** state) {
    test_state_t* ts = *state;
    assert_non_null(ts->ctx);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_v3_cycle, test_setup, test_teardown),
    };
    return cmocka_run_group_tests(tests, group_setup, group_teardown);
}
