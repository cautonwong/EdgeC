#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"

static void test_privacy_sandbox_toggle(void **state) {
    (void)state;
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    assert_non_null(hub);
    uv_loop_t loop;
    uv_loop_init(&loop);

    int rc = datahub_init(hub, &loop, "/test_hub_hw", 128, 15000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    // Write privacy tag = 1
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 12008, 1.0, 1000);
    rc = datahub_write_shadow_idx(hub, 12008, &msg);
    assert_int_equal(rc, 0);

    pipe_msg_t read_msg;
    rc = datahub_read_shadow(hub, 12008, &read_msg);
    assert_int_equal(rc, 0);
    assert_double_equal(read_msg.payload.val.f, 1.0, 0.0001);

    datahub_destroy(hub);
    free(hub);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_privacy_sandbox_toggle),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
