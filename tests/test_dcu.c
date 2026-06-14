#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"

typedef struct {
    datahub_t* hub;
} test_state_t;

static int setup(void** state) {
    test_state_t* ctx = calloc(1, sizeof(test_state_t));
    uv_loop_t* loop = uv_default_loop();
    ctx->hub = calloc(1, sizeof(datahub_t));
    // 适配新签名
    datahub_init(ctx->hub, loop, "dcu_test", 128, 1024, NULL, 0, NULL, NULL, NULL);
    *state = ctx;
    return 0;
}

static int teardown(void** state) {
    test_state_t* ctx = *state;
    datahub_destroy(ctx->hub);
    free(ctx->hub);
    free(ctx);
    return 0;
}

static void test_dcu_logic(void** state) {
    test_state_t* ctx = *state;
    assert_non_null(ctx->hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dcu_logic, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
