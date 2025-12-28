#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <uv.h>
#include <stdlib.h>

#include "svc_script.h"

// Mock Data
static PipeMsg g_msg = {.payload = {.val_f = 100.0}};

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

// Case 1: 正常执行
static void test_exec_normal(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    script_executor_t *exec = script_exec_create(&loop);
    assert_non_null(exec);

    assert_int_equal(script_exec_post(exec, "normal", SCRIPT_PRIO_ALARM, &g_msg), 0);

    usleep(50000);

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

    for (int i = 0; i < 128; i++) {
        script_exec_post(exec, "flood", SCRIPT_PRIO_ALGO, &g_msg);
    }

    assert_int_not_equal(script_exec_post(exec, "flood", SCRIPT_PRIO_ALGO, &g_msg), 0);

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