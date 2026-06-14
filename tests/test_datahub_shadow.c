#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"

static void test_shadow_read_consistency(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, &loop, "shadow_stress", 1024, 1024, NULL, 0, NULL, NULL, NULL);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 100, 123.45, 1000);
    datahub_publish(&hub, &msg);

    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(&hub, 100, &out), 0);
    assert_float_equal(out.payload.val.f, 123.45, 0.001);

    datahub_destroy(&hub);
    // 关键：排空并关闭 loop
    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_shadow_read_consistency),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}