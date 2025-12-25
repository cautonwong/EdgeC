#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "sqlite_service.h"
#include <unistd.h>
#include <stdio.h>

static void my_sql_cb(int rc, void *u) {
    assert_int_equal(rc, 0); // SQLITE_OK
    *(int *)u = 1;
}

static void test_sql_insert(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    // Remove old db if exists
    unlink("test.db");

    int done = 0;
    int ret = sqlite_exec_async(&loop,
        "CREATE TABLE IF NOT EXISTS t(a INT);",
        my_sql_cb,
        &done
    );
    assert_int_equal(ret, 0);

    // Run until done
    // UV_QUEUE_WORK involves a thread pool.
    int i;
    for (i=0; i<200; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (done) break;
        usleep(2000);
    }

    assert_int_equal(done, 1);
    
    // --- Graceful Shutdown ---
    // Ensure all work callbacks are processed
    uv_run(&loop, UV_RUN_DEFAULT);

    // Close loop
    int r = uv_loop_close(&loop);
    if (r == UV_EBUSY) {
        // Should not happen if we waited enough
        printf("Loop busy, forcing run...\n");
        uv_run(&loop, UV_RUN_DEFAULT);
        uv_loop_close(&loop);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sql_insert),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
