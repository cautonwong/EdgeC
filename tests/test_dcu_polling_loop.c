#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"

static datahub_t g_hub;
static int received_publish_count = 0;
static double last_received_val = 0.0;

static int _test_poller_publish(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                uint64_t timestamp_ns) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)timestamp_ns;
    received_publish_count++;
    last_received_val = val;
    return 0;
}

// 模拟抄表物理收发并推向 DataHub
static void test_polling_simulation(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    received_publish_count = 0;
    last_received_val = 0.0;

    // 1. 初始化 DataHub，作为总线载体
    int ret = datahub_init(&g_hub, loop, "dcu_bus", 128, 0, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 2. 初始化电表档案并增加 1 只表
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1, NULL);

    // 3. 初始化轮询调度引擎，注册定时任务与普通采集方案
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[1] = {OAD_VOLTAGE};  // 电压 OAD
    dcu_poller_add_scheme(&poller, 1, 0, oads, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);  // 0秒周期，确保立即执行

    poller_sink_ops_t poller_sink = {0};
    poller_bus_ops_t poller_bus = {
        .ctx = NULL, .publish_realtime = _test_poller_publish, .push_event = NULL};

    // 4. 启动动态轮询调度
    ret = dcu_poller_start(&poller, (poller_io_ops_t){0}, poller_bus, poller_sink);
    assert_int_equal(ret, 0);

    // 5. 调用数采数据泵，驱动一次模拟抄表
    dcu_poller_pump(&poller, &inv);

    // 6. 验证运行状态：确认通过契约接口向 DataHub 成功推送了数据
    assert_int_equal(received_publish_count, 1);
    assert_float_equal(last_received_val, 220.88f, 0.01f);

    // 销毁
    dcu_poller_stop(&poller);
    dcu_inventory_destroy(&inv);
    datahub_destroy(&g_hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_polling_simulation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
