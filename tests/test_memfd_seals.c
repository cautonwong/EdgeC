#define _GNU_SOURCE
#include <cmocka.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"

static void test_memfd_sealing_logic(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    // 适配新签名
    int ret = datahub_init(&hub, &loop, "test_seal", 64, 100, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    assert_true(hub.shadow_fd >= 0);
    int seals = fcntl(hub.shadow_fd, F_GET_SEALS);
    assert_true(seals & F_SEAL_SHRINK);
    assert_true(seals & F_SEAL_GROW);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_memfd_sealing_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}