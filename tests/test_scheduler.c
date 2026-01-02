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
static void on_fire(calendar_task_t *task) {
    (void)task;
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
    calendar_task_t t = {0};
    wall_time_t now = 1700043180000;  // 10:13:00 UTC

    t.type = CAL_TASK_MINUTE_FREEZE;
    assert_true(calc_next_boundary(&t, now) == now + 60000);

    t.type = CAL_TASK_PERIODIC_READ;
    t.align_min = 15;
    assert_true(calc_next_boundary(&t, now) == now + 120000);  // 10:15:00
}

static void test_clock_forward(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;

    calendar_task_t *task = calloc(1, sizeof(calendar_task_t));
    task->type = CAL_TASK_HOUR_FREEZE;
    task->loop = loop;
    task->on_fire = on_fire;

    mock_set_time(1700000000000, 1000);
    calendar_schedule(task);

    mock_set_time(1700014401000, 2000);
    calendar_schedule(task);

    assert_true(task->target_wall > 1700014401000);

    // 物理清理：手动触发 cancel (包含 close) 并释放内存
    calendar_cancel(task);

    // 给 close 回调执行的机会，然后释放堆内存
    uv_run(loop, UV_RUN_NOWAIT);
    free(task);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_calendar_alignments, setup, teardown),
        cmocka_unit_test_setup_teardown(test_clock_forward, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
