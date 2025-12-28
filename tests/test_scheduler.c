#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "sched_time.h"

// Mock control
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

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    
    // 强制关闭所有活跃句柄 (定时器等)
    uv_walk(loop, on_walk_close, NULL);
    
    // 驱动 Loop 直到所有 Close 回调完成
    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
    
    free(loop);
    return 0;
}

// Case 1: 时钟回拨 (合并自之前)
static void test_clock_backward(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    calendar_task_t task = {0};
    task.type = CAL_TASK_DAY_FREEZE;
    task.loop = loop;
    task.on_fire = on_fire;
    task.persist.last_fired_wall = 0;

    // 23:59
    mock_set_time(1700000000000, 1000);
    calendar_schedule(&task);  // Should schedule for tomorrow 00:00

    // Simulate Fired
    task.persist.last_fired_wall = task.target_wall;

    // Back to 23:00
    mock_set_time(1699996400000, 2000);
    calendar_schedule(&task);  // Should SKIP because last_fired >= next_boundary

    // 我们通过检查 task.state 是否变为 SCHEDULED 来验证
    // 如果跳过了，state 应该是 DONE (我们在 schedule 里设为 DONE)
    // 或者是保持之前的状态。这里 calendar_schedule 如果跳过会设为 DONE。
    assert_int_equal(task.state, CAL_STATE_DONE);
}

// Case 2: 时钟快进 (不补跑)
static void test_clock_forward(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    calendar_task_t task = {0};
    task.type = CAL_TASK_HOUR_FREEZE;
    task.loop = loop;
    task.on_fire = on_fire;

    // 01:00
    mock_set_time(1700000000000, 1000);
    calendar_schedule(&task);
    
    // Fast forward to 05:00:01
    mock_set_time(1700014401000, 2000);
    
    calendar_schedule(&task);
    
    // 下一个整点边界应为 06:00:00 UTC
    // 1700018000000
    assert_int_equal(task.target_wall, 1700018000000);

    // Run loop briefly, verify NO fire
    uv_run(loop, UV_RUN_NOWAIT);
    assert_int_equal(fire_count, 0);
}

// Case 3: 边界对齐 (Alignment)

static void test_alignment(void **state) {

    (void)state;

    // 使用一个确定的 Unix 时间戳 (例如 2023-11-15 10:13:00 UTC)

    // 1700043180 = 2023-11-15 10:13:00

    wall_time_t now = 1700043180000; 

    

    wall_time_t next = calc_next_boundary(CAL_TASK_PERIODIC_READ, now);

    

    // Expect 10:15:00 -> 1700043300000

    assert_int_equal(next, 1700043300000);

}



int main(void) {

    const struct CMUnitTest tests[] = {

        cmocka_unit_test_setup_teardown(test_clock_backward, setup, teardown),

        cmocka_unit_test_setup_teardown(test_clock_forward, setup, teardown),

        cmocka_unit_test(test_alignment),

    };

    return cmocka_run_group_tests(tests, NULL, NULL);

}


