#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "sched_time.h"

void mock_set_time(wall_time_t w, mono_time_t m);

static int fire_count = 0;
static void on_periodic_cb(uv_periodic_t *handle) {
    (void)handle;
    fire_count++;
}

static int setup(void **state) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    fire_count = 0;
    return 0;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;

    uv_walk(loop, on_walk_close, NULL);

    // 受控排干：最多跑 10 轮
    for (int i = 0; i < 10; i++) uv_run(loop, UV_RUN_NOWAIT);

    uv_loop_close(loop);
    free(loop);
    return 0;
}

static void test_calendar_alignments(void **state) {
    (void)state;
    uv_periodic_t t = {0};
    wall_time_t now = 1700043180000;  // 10:13:00 UTC

    // 1. 测试整分对齐
    t.reschedule_cb = sched_policy_at_minute;
    t.offset_ms = 0;
    assert_true(t.reschedule_cb(&t, now) == now + 60000);

    // 2. 测试 15 分钟对齐
    t.reschedule_cb = NULL;
    t.interval_ms = 15 * 60 * 1000;
    t.offset_ms = 1700006400000;                                  // 假设这是当天零点
    assert_true(sched_policy_aligned(&t, now) == 1700043300000);  // 10:15:00
}

static void test_clock_forward(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;

    uv_periodic_t *handle = calloc(1, sizeof(uv_periodic_t));
    handle->reschedule_cb = sched_policy_at_hour;
    handle->loop = loop;

    mock_set_time(1700000000000, 1000);
    uv_periodic_start(handle, on_periodic_cb);

    mock_set_time(1700014401000, 2000);
    uv_periodic_start(handle, on_periodic_cb);

    assert_true(handle->target_wall > 1700014401000);

    uv_periodic_stop(handle);
    uv_run(loop, UV_RUN_NOWAIT);
    free(handle);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_calendar_alignments, setup, teardown),
        cmocka_unit_test_setup_teardown(test_clock_forward, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
