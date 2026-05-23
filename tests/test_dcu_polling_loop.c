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

static datahub_t g_hub;
static bool g_received_telemetry = false;
static double g_received_value = 0.0;

// DataHub 唤醒/刷新回调，当总线数据发布时被触发
static void on_datahub_wakeup(datahub_t *hub, void *user_data) {
    (void)hub;
    (void)user_data;
    
    // 监听我们的目标测点 20000200
    // 在 DataHub 中，遥测数据通常映射在对应的 tag 上。
    // 为了极简测试，我们可以订阅 DataHub 发布出来的事件。
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

    // 3. 初始化轮询调度引擎，注册 10ms 高频电压方案
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[1] = {0x20000200}; // 电压 OAD
    dcu_poller_add_scheme(&poller, 1, 10, oads, 1); // 10ms 快速周期

    // 4. 启动动态轮询线程/uv_timer
    ret = dcu_poller_start_simulation(&poller, &inv, &g_hub, loop);
    assert_int_equal(ret, 0);

    // 5. 运行 libuv 循环 35ms，让 10ms 的定时器触发数次并生产数据
    uint64_t start_ms = uv_now(loop);
    while (uv_now(loop) - start_ms < 35) {
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
