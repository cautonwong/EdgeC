#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>
#include <uv.h>
#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"

static datahub_t g_hub;

// DataHub 唤醒/刷新回调，当总线数据发布时被触发
static void on_datahub_wakeup(datahub_t *hub, void *user_data) {
    (void)hub;
    (void)user_data;
}

// 模拟抄表物理收发并推向 DataHub 
static void test_polling_simulation(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    // 1. 初始化 DataHub，作为总线载体
    int ret = datahub_init(&g_hub, loop, "dcu_bus", 128, 0, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 2. 初始化电表档案并增加 1 只表
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);

    // 3. 初始化轮询调度引擎，注册定时任务与普通采集方案
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[1] = {OAD_VOLTAGE}; // 电压 OAD
    dcu_poller_add_scheme(&poller, 1, 0, oads, 1);
    dcu_poller_add_task(&poller, 1, 1, 1); // 1秒周期

    // 4. 启动动态轮询线程/uv_timer
    ret = dcu_poller_start_simulation(&poller, &inv, &g_hub, loop);
    assert_int_equal(ret, 0);

    // 5. 运行 libuv 循环 1050ms，让对齐周期定时器触发并生产数据
    uint64_t start_ms = uv_now(loop);
    while (uv_now(loop) - start_ms < 1050) {
        uv_run(loop, UV_RUN_ONCE);
    }


    // 6. 验证 DataHub 状态：
    // 我们从 DataHub 或者是 poller 产生的数据中，确认发布动作已经成功发生。
    // 在 dcu_poller_start_simulation 内部，我们会产生 220V 左右的浮点电压并执行 datahub_publish_scalar
    // 我们可以直接读取 poller 的最新值，或者断言其模拟器动作数次
    extern uint32_t g_simulated_count;
    assert_true(g_simulated_count > 0);

    // 销毁
    dcu_poller_stop_simulation(&poller);
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
    datahub_destroy(&g_hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_polling_simulation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
