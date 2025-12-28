#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"

static void my_sql_cb(int rc, void *u) {
    assert_int_equal(rc, 0);
    *(int *)u = 1;
}

static void test_sql_insert(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    unlink("test.db");

    int done = 0;
    int ret = sqlite_exec_async(&loop, "CREATE TABLE IF NOT EXISTS t(a INT);", my_sql_cb, &done);
    assert_int_equal(ret, 0);

    int i;
    for (i = 0; i < 200; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (done) break;
        usleep(2000);  // libuv sleep
    }

    assert_int_equal(done, 1);

    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sql_insert),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}