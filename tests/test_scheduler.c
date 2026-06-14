#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_common.h"
#include "sched_time.h"

// 模拟时间函数声明
void mock_set_time(wall_time_t w, mono_time_t m);

static void test_calendar_alignments(void** state) {
    (void)state;
    uv_periodic_t t;
    uv_periodic_init(uv_default_loop(), &t);

    // 假设现在是 10:13:00 (ms)
    wall_time_t now = 1700043180000;

    // 策略测试：对齐到小时

    double next_hour = sched_policy_at_hour(&t, (double)now);

    assert_int_equal((int64_t)next_hour, 1700046000000);  // 11:00:00

    // 策略测试：对齐到分钟

    double next_min = sched_policy_at_minute(&t, (double)now);

    assert_int_equal((int64_t)next_min, 1700043240000);  // 10:14:00
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_calendar_alignments),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}