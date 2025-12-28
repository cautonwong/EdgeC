#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "sched_time.h"

// External mock control
void mock_set_time(wall_time_t w, mono_time_t m);

static int fire_count = 0;

static void on_fire(calendar_task_t *task) {
    (void)task;
    fire_count++;
}

static void test_clock_backward_no_double_fire(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    calendar_task_t task = {0};
    task.type = CAL_TASK_DAY_FREEZE;
    task.loop = &loop;
    task.on_fire = on_fire;
    task.persist.last_fired_wall = 0;

    // 1. Initial State: 2023-11-15 23:59:00 (Approx timestamp)
    // Wall: 1700092740000
    wall_time_t t0 = 1700092740000;
    mock_set_time(t0, 1000);

    // Schedule
    calendar_schedule(&task);
    assert_int_equal(task.state, CAL_STATE_SCHEDULED);

    // Run loop - should NOT fire yet (1 min remaining)
    uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(fire_count, 0);

    // 2. Advance Mono Time to boundary (Simulate time passing)
    // Next day boundary from 23:59 is +60s = +60000ms
    mock_set_time(t0 + 60000, 1000 + 60000);

    // We need to simulate the timer firing.
    // In a real loop, libuv checks mono time.
    // Since we mocked sched_mono_now, libuv doesn't know it changed unless we sleep or it polls
    // system clock. BUT we are using real libuv. Real libuv uses system CLOCK_MONOTONIC. Our mock
    // only affects the scheduler's *calculation* of delta. The uv_timer relies on OS kernel
    // monotonic time. Mismatched Mock vs Real UV: If we want to test logic, we should rely on the
    // calculated `target_wall` and `state`. Actually running the timer requires waiting real time
    // or mocking libuv itself. For this unit test, we trust the `calendar_schedule` logic: It
    // calculated `delta` based on `mock_wall`. If we want to verify "No Double Fire", we simulate
    // the restart.

    // Let's manually trigger fire to update state, as if time passed
    // (Simulating the callback execution)
    // Force state update
    task.on_fire(&task);
    task.persist.last_fired_wall = task.target_wall;
    task.state = CAL_STATE_DONE;
    fire_count = 1;  // Manually set for tracking

    // 3. Clock Backward Scenario
    // Wall clock goes BACK to 23:00
    wall_time_t t_back = t0 - 3600000;
    mock_set_time(t_back, 200000);  // Mono time keeps moving forward

    // Re-schedule (e.g. periodic check or reboot)
    calendar_schedule(&task);

    // Assertions:
    // Should NOT schedule again because `last_fired_wall` (Tomorrow 00:00) >= `next_wall` (Tomorrow
    // 00:00 computed from 23:00) Wait, if now is 23:00, next boundary is Tomorrow 00:00.
    // last_fired_wall is Tomorrow 00:00.
    // last >= next ? True.
    // So it should skip.

    assert_int_equal(task.state, CAL_STATE_DONE);  // State shouldn't change to SCHEDULED

    // Cleanup
    if (!uv_is_closing((uv_handle_t *)&task.timer)) {
        uv_close((uv_handle_t *)&task.timer, NULL);
    }
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_clock_backward_no_double_fire),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
