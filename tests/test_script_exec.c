#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "svc_script.h"

static PipeMsg g_msg = {.payload = {.val = {.f = 100.0}}};

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void test_exec_normal(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    script_executor_t *exec = script_exec_create(&loop);
    assert_non_null(exec);
    assert_int_equal(script_exec_post(exec, "normal", SCRIPT_PRIO_ALARM, &g_msg), 0);
    usleep(10000);
    script_exec_destroy(exec);
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

static void test_queue_overflow(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    script_executor_t *exec = script_exec_create(&loop);
    assert_non_null(exec);

    int success_count = 0;
    int fail_count = 0;

    // Push many tasks rapidly to overwhelm the consumer
    for (int i = 0; i < 1000; i++) {
        if (script_exec_post(exec, "non_existent_script", SCRIPT_PRIO_ALGO, &g_msg) == 0) {
            success_count++;
        } else {
            fail_count++;
        }
    }

    // We expect at least some failures when pushing 1000 items into a 128-slot queue
    assert_true(fail_count > 0);

    script_exec_destroy(exec);
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_exec_normal),
        cmocka_unit_test(test_queue_overflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
